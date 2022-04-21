#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include <sys/file.h>
#include "coord.grpc.pb.h"
#include "service.grpc.pb.h"
//#include "fileaccess.h"

using google::protobuf::Timestamp;
using google::protobuf::Duration;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

using coord::SNSCoordinator;
using coord::ClientRequest;
using coord::ClientReply;
using coord::HeartBeat;
using coord::MASTER;
using coord::SLAVE;

using service::Message;
using service::Request;
using service::Reply;
using service::SNSServer;
using google::protobuf::util::TimeUtil;

std::mutex mu_;
std::unordered_map<std::string, ServerReaderWriter<Message,Message>*> umap;
std::string prefix = "userfiles/";

struct f_user{
  std::string username;
  Timestamp timestamp;
};

// int filelock(std::string filename){
//   int fd = open(filename.c_str(),O_RDONLY);
//     if (flock(fd, LOCK_EX) == -1) {
//         std::cout<<"lock failed"<<std::endl;
//         exit(1);
//     }
//     return fd;
// }

// void fileunlock(int fd){
//   if (flock(fd, LOCK_UN) == -1) {
//         std::cout<<"unlock failed"<<std::endl;
//         exit(1);
//   }
// }

std::vector<std::string> getUsersFromFile(std::string filename){
  //std::string prefix = "userFiles/";
  std::vector<std::string> vect;
  filename = prefix + filename;
  //int fd = filelock(filename);
  std::ifstream infile;
  infile.open(filename);
  std::string line;
  while (getline(infile, line)){
      //std::cout<<line<<std::endl;
      vect.push_back(line);
  }
  infile.close();
  //fileunlock(fd);
  std::cout<<"got users from "<<filename<<std::endl;
  return vect;
  
}

bool doesUserExistInFile(std::string username,std::string filename){
    //std::string prefix = "userFiles/";
    std::vector<std::string> vect;
    vect = getUsersFromFile(filename);
    filename = prefix + filename;
    
    
    std::string key = username;
    if (std::count(vect.begin(), vect.end(), key)) {
        std::cout << "User found in "<<filename<<std::endl;
        return true;
    }
    else {
        std::cout << "User not found in "<<filename<<std::endl;
        return false;
  
  }
}

void addUsertoFile(std::string username,std::string filename){
  
  //std::string prefix = "userFiles/";
  filename = prefix+filename;
  
  std::cout<<"added "<<username<<" to "<<filename<<std::endl;
  std::string key = username;
  //int fd = filelock(filename);
  std::ofstream outfile;
  outfile.open(filename,std::ios_base::app);
  outfile << key<< std::endl;
  outfile.close();
  //fileunlock(fd);
}

void addUserFiles(std::string username){
  //std::string prefix = "userFiles/";
  std::string key = username;
  std::ofstream outfile;
  outfile.open(prefix+key+"_mytimeline.txt");
  outfile.close();
  outfile.open(prefix+key+"_othertimeline.txt");
  outfile.close();
  outfile.open(prefix+key+"_followers.txt");
  outfile.close();
  outfile.open(prefix+key+"_following.txt");
  outfile.close();
  std::cout << "User files added"<<std::endl;
  
}

void removeUserFromFile(std::string username,std::string filename){
  //std::string prefix = "userFiles/";
  std::vector<std::string> vect;
  vect = getUsersFromFile(filename);
  filename = prefix + filename;
  ////int fd = filelock(filename);
  
  std::ofstream outfile;
  outfile.open(filename);
  for (std::vector<std::string>::iterator t = vect.begin(); t != vect.end(); ++t) 
  {
      //std::cout<<*t<<std::endl;
      std::string element = t->c_str();
      if (element != username){
          outfile<<element<<std::endl;
      }
  }
  
}

void addPostToFile(Message note, std::string filename){
  //std::string prefix = "userFiles/";
  filename = prefix+filename;
  //int fd = filelock(filename);
  std::ofstream outfile;
  outfile.open(filename,std::ios_base::app);
  outfile << note.username()<< std::endl;
  outfile << note.msg()<< std::endl;
  std::string strStamp = TimeUtil::ToString(note.timestamp());
  outfile << strStamp<< std::endl;
  outfile.close();
  //fileunlock(fd);
  
}

void addFollowToFile(std::string username, std::string filename){
  //std::string prefix = "userFiles/";
  auto timestamp = new google::protobuf::Timestamp{};
  timestamp->set_seconds(time(NULL));
  timestamp->set_nanos(0);
  std::string strStamp = TimeUtil::ToString(*timestamp);
  filename = prefix+filename;
  //int fd = filelock(filename);
  std::ofstream outfile;
  outfile.open(filename,std::ios_base::app);
  outfile << username<< std::endl;
  outfile << strStamp<< std::endl;
  outfile.close();
  //fileunlock(fd);
  
}

std::vector<f_user> getFollowsFromFile(std::string filename){
  //std::string prefix = "userFiles/";
  std::vector<f_user> vect;
  filename = prefix + filename;
  //int fd = filelock(filename);
  std::ifstream infile;
  infile.open(filename);
  std::string line;
  
  while (getline(infile, line)){
      //std::cout<<line<<std::endl;
      f_user user;
      std::string username;
      user.username = line;
      getline(infile,line);
      auto t = new Timestamp{};
      
      if (TimeUtil::FromString(line,t)){
        user.timestamp = *t;
      }
      vect.push_back(user);
  }
  infile.close();
  //fileunlock(fd);
  return vect;
  
}



std::vector<Message> getPostsFromFile(std::string filename){
  //std::string prefix = "userFiles/";
  std::vector<Message> vect;
  filename = prefix + filename;
  //int fd = filelock(filename);
  std::ifstream infile;
  infile.open(filename);
  std::string line;
  
  while (getline(infile, line)){
      //std::cout<<line<<std::endl;
      Message post;
      post.set_username(line);
      getline(infile,line);
      post.set_msg(line);
      getline(infile,line);
      auto t = new Timestamp{};
      
      if (TimeUtil::FromString(line,t)){
        post.set_allocated_timestamp(t);
      }
      vect.push_back(post);
  }
  infile.close();
  //fileunlock(fd);
  return vect;
  
}

bool my_cmp(const Message& a, const Message& b)  {
      // smallest comes first
      return a.timestamp() < b.timestamp();
  }

std::vector<Message> getAllPosts(std::string username){
  //std::string prefix = "userFiles/";
  std::vector<Message> myPosts;
  std::vector<Message> otherPosts;
  std::vector<Message> allPosts;
  std::string myPostsFile = username+"_mytimeline.txt";
  std::string otherPostsFile = username+"_othertimeline.txt";

  myPosts = getPostsFromFile(myPostsFile);
  std::cout<<"myposts before"<<myPosts.size()<<std::endl;
  otherPosts = getPostsFromFile(otherPostsFile);
  std::cout<<"otherposts before"<<otherPosts.size()<<std::endl;
  //combine self posts and other users posts
  allPosts = myPosts;
  allPosts.insert(allPosts.end(), otherPosts.begin(), otherPosts.end());

  
  //sort them in descending order based on timestamp
  std::sort(allPosts.begin(), allPosts.end(), my_cmp);
  std::cout<<"allposts before"<<allPosts.size()<<std::endl;
  //put following into a map (key is username, timestamp is value) for quicker access
  std::vector<f_user> following = getFollowsFromFile(username+"_following.txt");
  std::map<std::string, Timestamp> followingMap;
  for (std::vector<f_user>::iterator t = following.begin(); t != following.end(); ++t){
    followingMap.insert({t->username,t->timestamp});
  }

  for (std::vector<Message>::iterator t = allPosts.begin(); t != allPosts.end(); ++t){
          if (t->username() != username && t->timestamp() < followingMap[t->username()]){
            allPosts.erase(t);
          }
      }
    std::cout<<"allposts"<<allPosts.size()<<std::endl;

  return allPosts;
  
}

void removePostsfromUserTimeline(std::string username, std::string unfollowed){
  std::vector<Message> userPosts = getPostsFromFile(username+"_timeline.txt");
  std::ofstream outfile;
  outfile.open(prefix+username+"_timeline.txt");
  outfile.close();
  for (std::vector<Message>::reverse_iterator it = userPosts.rbegin(); it != userPosts.rend(); it++){
    auto post = *it;
    if (post.username() != unfollowed){
      addPostToFile(post,username+"_timeline.txt");
    }
  }
  
}

std::string coordinatorIP;
std::string coordinatorPort;
std::string port;
std::string id;
std::string type;
std::unique_ptr<coord::SNSCoordinator::Stub> stub_;
std::unique_ptr<service::SNSServer::Stub> slave_stub_;
std::string slavePort;
std::string slaveIP;




class SNSServerImpl final : public SNSServer::Service {
  int hello = 9;
  
  
  Status List(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // LIST request from the user. Ensure that both the fields
    // all_users & following_users are populated
    // ------------------------------------------------------------
    std::cout<<"start list procedure"<<std::endl;
    std::string username = request->username();
    std::string message = "0";
    std::vector<std::string> allUsers;
    std::vector<std::string> followers;
    if (doesUserExistInFile(username,"myusers.txt") || doesUserExistInFile(username,"otherusers.txt")){
      std::vector<std::string> myUsers = getUsersFromFile("myusers.txt");
      std::vector<std::string> otherUsers = getUsersFromFile("otherusers.txt");
      allUsers = myUsers;
      allUsers.insert(allUsers.end(), otherUsers.begin(), otherUsers.end());
      
      followers = getUsersFromFile(username+"_followers.txt");
      for (std::vector<std::string>::iterator t = allUsers.begin(); t != allUsers.end(); ++t){
          std::string element = t->c_str();
          reply->add_all_users(element);
      }
      for (std::vector<std::string>::iterator t = followers.begin(); t != followers.end(); ++t){
          std::string element = t->c_str();
          reply->add_following_users(element);
      }
      message = "1";
      
    }
    else{
      message = "4";
    }
    reply->set_msg(message);
    return Status::OK;
  }

  Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // request from a user to follow one of the existing
    // users
    // ------------------------------------------------------------
    if (type == "master"){
      if (!slavePort.empty()){
        grpc::ClientContext sendslavecontext;
        grpc::Status sendSlaveStatus = slave_stub_->Follow(&sendslavecontext,*request,&*reply);
        //ire.grpc_status = sendSlaveStatus;
        if (sendSlaveStatus.ok()) {
            //ire.comm_status = SUCCESS;
            std::cout<<"send follow to slave ok"<<reply->msg()<<std::endl;
        } else {
            //ire.comm_status = FAILURE_NOT_EXISTS;
            std::cout<<"send follow to slave not ok"<<std::endl;
        }


      }
    }
    std::cout<<"begin follow procedure"<<std::endl;
    std::string username = request->username();
    std::string followed = request->arguments(0);
    std::string message = "0";
    if ((doesUserExistInFile(followed,"myusers.txt") || doesUserExistInFile(followed,"otherusers.txt")) && username != followed){
      if (doesUserExistInFile(followed,username+"_following.txt")==false){
        //addUsertoFile(username,followed+"_followers.txt");
        addFollowToFile(followed,username+"_following.txt");
        message = "1";
      }
      else{
        message = "2";
      }
    }
    else{
      message = "4";
    }
    
    reply->set_msg(message);
    
    return Status::OK; 
  }

  // Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {
  //   // ------------------------------------------------------------
  //   // In this function, you are to write code that handles 
  //   // request from a user to unfollow one of his/her existing
  //   // followers
  //   // ------------------------------------------------------------
  //   std::string username = request->username();
  //   std::string followed = request->arguments(0);
  //   std::string message = "0";
  //   if (doesUserExistInFile(followed,"users.txt") && username != followed){
  //     if (doesUserExistInFile(username,followed+"_followers.txt")==true && doesUserExistInFile(followed,username+"_following.txt")==true){
  //       removeUserFromFile(username,followed+"_followers.txt");
  //       removeUserFromFile(followed,username+"_following.txt");
  //       removePostsfromUserTimeline(username,followed);
  //       message = "1";
  //     }
  //     else{
  //       message = "3";
  //     }
  //   }
  //   else{
  //     message = "4";
  //   }
    
  //   reply->set_msg(message);
    
  //   return Status::OK;
  // }
  
  Status Login(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // a new user and verify if the username is available
    // or already taken
    // ------------------------------------------------------------
    std::string username = request->username();
    std::string returnString = "got client"+username;
    std::cout<<returnString<<std::endl;

    //master needs to get slave ip/port if not already known.
    //master needs to send message from client to slave
    if (type == "master"){
      std::cout<<"if1"<<std::endl;
      if (slavePort.empty() && slaveIP.empty()){
        std::cout<<"if2"<<std::endl;
        ClientRequest slaveReq;
        ClientReply slaveRep;
        grpc::ClientContext getslavecontext;
        slaveReq.set_id(stoi(id));

        grpc::Status slaveStatus = stub_->GetSlave(&getslavecontext,slaveReq,&slaveRep);
            //ire.grpc_status = slaveStatus;
            if (slaveStatus.ok()) {
                //ire.comm_status = SUCCESS;
                if (slaveRep.ip() != "NaN" && slaveRep.port()!= "NaN"){
                  std::cout<<"slave received"<<slaveRep.port()<<std::endl;
                  slavePort = slaveRep.port();
                  slaveIP = slaveRep.ip();
                  std::string s = slaveRep.ip()+":"+slaveRep.port();
                  auto channel = grpc::CreateChannel(s, grpc::InsecureChannelCredentials());
                  slave_stub_ = service::SNSServer::NewStub(channel);

                }
                else{
                  std::cout<<"slave not received"<<std::endl;
                }
            } else {
                //ire.comm_status = FAILURE_NOT_EXISTS;
                std::cout<<"get slave: coordinator issue"<<std::endl;
            }
      }
      if (!slavePort.empty()){
        grpc::ClientContext sendslavecontext;
        grpc::Status sendSlaveStatus = slave_stub_->Login(&sendslavecontext,*request,&*reply);
        //ire.grpc_status = sendSlaveStatus;
        if (sendSlaveStatus.ok()) {
            //ire.comm_status = SUCCESS;
            std::cout<<"send to slave ok"<<reply->msg()<<std::endl;
        } else {
            //ire.comm_status = FAILURE_NOT_EXISTS;
            std::cout<<"send to slave not ok";
        }


      }
    }
    std::cout<<"starting login procedure"<<std::endl;

    std::string userfile = "myusers.txt";
    if (doesUserExistInFile(username,userfile) == false){
      addUsertoFile(username,userfile);
      
      addUserFiles(username);
      //addUsertoFile(username,username+"_followers.txt");
      //addUsertoFile(username,username+"_following.txt");
      
    }
    
    
    
    
    reply->set_msg(returnString);
    
    return Status::OK;
  }

  Status Timeline(ServerContext* context, ServerReaderWriter<Message, Message>* stream) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // receiving a message/post from a user, recording it in a file
    // and then making it available on his/her follower's streams
    // ------------------------------------------------------------
    Message note;
    std::string username;
    std::vector<std::string> followers;
    //std::cout<<"server start"<<std::endl;
    // while (stream->Read(&note)) {
    //   //std::unique_lock<std::mutex> lock(mu_);
      
    //   //stream->Write(note);
    //   username = note.username();
    //   std::vector<std::string> followers;
    //   std::vector<std::string> following;
      
    //   if (umap.find(username) == umap.end()){
    //     following = getUsersFromFile(username+"_following.txt");
    //     std::unordered_set<std::string> followingSet(following.begin(), following.end());
    //     umap[username]=stream;
    //     std::vector<Message> posts = getPostsFromFile(username+"_timeline.txt");
    //     int count = 0;
    //     for (std::vector<Message>::reverse_iterator it = posts.rbegin(); it != posts.rend(); it++){
    //       if (count == 20){
    //         break;
    //       }
    //       else if (followingSet.find((*it).username())!=followingSet.end()){
    //         stream->Write(*it);
    //         count++;
    //       }
    //     }
        
    //   }
    //   else{
    //   followers = getUsersFromFile(username+"_followers.txt");
    //   for (std::vector<std::string>::iterator t = followers.begin(); t != followers.end(); ++t){
    //     std::string element = t->c_str();
    //     addPostToFile(note,element+"_timeline.txt");
    //     if (umap.find(element) != umap.end() && element != username){
    //       auto newStream = umap[element];
    //       newStream->Write(note);
    //     }
    //   }
    //   }
        
        
        
        
    // }
    // if (umap.find(username) != umap.end()){
    //   umap.erase(username);
    // }
    // std::cout<<"server end erased "<<username<<std::endl;

    //new implementation

    std::shared_ptr<grpc::ClientReaderWriter<Message, Message> > slavestream;
    grpc::ClientContext slavecontext;
    std::cout<<"starting timeline"<<std::endl;
    if (type == "master"){
      slavestream=slave_stub_->Timeline(&slavecontext);
    }
    

    while (stream->Read(&note)) {
      if (type == "master"){slavestream->Write(note);}
      username = note.username();
      if (!username.empty()){
        break;
      }
    }

    std::thread timelinewriter([stream,username]() {
       std::time_t begin = std::time(0);  
       while(1){
        std::vector<Message> orderedPosts = getAllPosts(username); 
        int count = 0;
        for (std::vector<Message>::reverse_iterator it = orderedPosts.rbegin(); it != orderedPosts.rend(); it++){
          if (count == 20){
            break;
          }
          stream->Write(*it);
          count++;
        }
        std::this_thread::sleep_for(std::chrono::seconds(30)); 
       }
        
    });
    while (stream->Read(&note)) {
      if (type == "master"){slavestream->Write(note);}
      username = note.username();
      addPostToFile(note,username+"_mytimeline.txt");
    }
    timelinewriter.join();

    


    

    return Status::OK;
  }

};

void RunServer(std::string port_no) {

  std::ifstream userFile;
  userFile.open(prefix+"myusers.txt");
  if (userFile){
    std::cout<<"userfile exists"<<std::endl;
    userFile.close();
  }
  else{
     std::cout<<"userfile does not exists"<<std::endl;
     std::ofstream outfile;
     outfile.open(prefix+"myusers.txt");
    outfile.close();
    outfile.open(prefix+"otherusers.txt");
    outfile.close();
    std::cout<<"userfiles created"<<std::endl;
  }
  std::string server_address = "localhost:"+port_no;
  SNSServerImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char** argv) {
  
  
  coordinatorIP = "localhost";
  coordinatorPort = "3010";
  port = "9190";
  id = "1";
  type = "master";
  int opt = 0;
  while ((opt = getopt(argc, argv, "c:p:m:i:t:")) != -1){
    switch(opt) {
        case 'c':
          coordinatorIP = optarg;break;
        case 'p':
          coordinatorPort = optarg;break;
      case 'm':
          port = optarg;break;
        case 'i':
          id = optarg;break;
        case 't':
          type = optarg;break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  //prefix is used everywhere to ensure 
  std::string output = "logfiles/"+type+id+"_output.txt";
  freopen(output.c_str(),"w",stdout);
  prefix = prefix+"s"+id+"_"+type+"_";
  

  std::stringstream ss;
	ss << coordinatorIP << ':' << coordinatorPort;
	std::string s = ss.str();
  
  auto channel = grpc::CreateChannel(s, grpc::InsecureChannelCredentials());
  stub_ = coord::SNSCoordinator::NewStub(channel);

  grpc::ClientContext context;

    std::shared_ptr<grpc::ClientReaderWriter<HeartBeat,HeartBeat>> stream(
    stub_->ServerCommunicate(&context));
    HeartBeat heartbeat;
    heartbeat.set_sid(stoi(id));
    if (type == "master"){
        heartbeat.set_s_type(MASTER);
    }
    else{
       heartbeat.set_s_type(SLAVE); 
    }
    heartbeat.set_ip("localhost");
    heartbeat.set_port(port);
    
    std::thread writer([stream,heartbeat]() {
       std::time_t begin = std::time(0);  
       while(1){       
       stream->Write(heartbeat);
       std::this_thread::sleep_for(std::chrono::seconds(10)); 
       }
        
    });

    RunServer(port);

    writer.join();


  return 0;
}