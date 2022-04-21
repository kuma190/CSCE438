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
#include <sys/file.h>
#include <unordered_map>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>

#include "coord.grpc.pb.h"
#include "synch.grpc.pb.h"
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
using coord::SYNCHRONIZER;

// using service::Message;
// using service::Request;
// using service::Reply;
// using service::SNSServer;
using google::protobuf::util::TimeUtil;

using synch::syncReq;
using synch::syncRep;
using synch::Message;
using synch::SNSSynchronizer;

struct f_user{
  std::string username;
  Timestamp timestamp;
};

struct entry{
    std::string ip;
    std::string port;
    std::unique_ptr<synch::SNSSynchronizer::Stub> sync_stub_;
};

std::unordered_map<std::string, int> filePlaces;
std::string prefix = "userfiles/";
entry fsyncs[4];

std::string coordinatorIP;
std::string coordinatorPort;
std::string port;
std::string id;
std::unique_ptr<coord::SNSCoordinator::Stub> stub_;

//std::string slavePort;
//std::string slaveIP;

// int filelock(std::string filename){
//   int fd = open(filename.c_str(),O_RDONLY);
//   std::cout<<filename<<" lock"<<std::endl;
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
  return vect;
  
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

int getFsyncId(int clientId){
  ClientRequest fsyncReq;
  ClientReply fsyncRep;
  grpc::ClientContext getfsynccontext;
  fsyncReq.set_id(clientId);
  grpc::Status fsyncStatus = stub_->GetFsync(&getfsynccontext,fsyncReq,&fsyncRep);
      //ire.grpc_status = fsyncStatus;
      if (fsyncStatus.ok()) {
          //ire.comm_status = SUCCESS;
          if (fsyncRep.ip() != "NaN" && fsyncRep.port()!= "NaN"){
            std::cout<<"fsync received"<<fsyncRep.port()<<std::endl;
            return fsyncRep.id();
          }
      }
      return -1;


}

 void getFsyncs(){
     for (int i = 1; i <=3; i++){
        if (!fsyncs[i].ip.empty()){
            continue;
        }
        ClientRequest fsyncReq;
        ClientReply fsyncRep;
        grpc::ClientContext getfsynccontext;
        fsyncReq.set_id(i-1);
        grpc::Status fsyncStatus = stub_->GetFsync(&getfsynccontext,fsyncReq,&fsyncRep);
            //ire.grpc_status = fsyncStatus;
            if (fsyncStatus.ok()) {
                //ire.comm_status = SUCCESS;
                if (fsyncRep.ip() != "NaN" && fsyncRep.port()!= "NaN"){
                  std::cout<<"fsync received"<<fsyncRep.port()<<std::endl;
                  fsyncs[i].port = fsyncRep.port();
                  fsyncs[i].ip = fsyncRep.ip();
                  std::string s = fsyncRep.ip()+":"+fsyncRep.port();
                  auto channel = grpc::CreateChannel(s, grpc::InsecureChannelCredentials());
                  fsyncs[i].sync_stub_ = synch::SNSSynchronizer::NewStub(channel);

                }
                else{
                  std::cout<<"fsync not received"<<std::endl;
                }
            } else {
                //ire.comm_status = FAILURE_NOT_EXISTS;
                std::cout<<"get fsync: coordinator issue"<<std::endl;
            }
     }

 }

 std::ifstream getFilePos(std::string filename){
    if (filePlaces.find(filename) == filePlaces.end()){
        filePlaces.insert({filename,0});
    }
    //start where read ended last time
    std::ifstream infile;
    infile.open(filename);
    infile.seekg(filePlaces[filename],std::ios::beg);
    return infile;
 }

 void setFilePos(std::string filename){
     //get start position of next read by opening ofstream in append mode
     //int fd = filelock(filename);
    std::ofstream outfile;
    outfile.open(filename,std::ios_base::app);
    filePlaces[filename] = outfile.tellp();
    outfile.close();
    //fileunlock(fd);

 }

 std::vector<std::string> getNewUsersFromFile(std::string filename){
  //std::string prefix = "userFiles/";

  std::vector<std::string> vect;
  filename = prefix+filename;
  //int fd = filelock(filename);
  std::ifstream infile = getFilePos(filename);


  std::string line;
  while (getline(infile, line)){
      //std::cout<<line<<std::endl;
      vect.push_back(line);
  }
  infile.close();
  //fileunlock(fd);
  setFilePos(filename);

  return vect;
  
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

std::vector<f_user> getNewFollowsFromFile(std::string filename){
  //std::string prefix = "userFiles/";
  std::vector<f_user> vect;
  filename = prefix + filename;
  //int fd = filelock(filename);
  std::ifstream infile = getFilePos(filename);

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
  setFilePos(filename);

  return vect;
  
}

std::vector<Message> getNewPostsFromFile(std::string filename){
  //std::string prefix = "userFiles/";
  std::vector<Message> vect;
  filename = prefix + filename;
  //int fd = filelock(filename);
  std::ifstream infile = getFilePos(filename);
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
  setFilePos(filename);  

  return vect;
  
}



void performSync(){
    //get all active fsyncs
    getFsyncs();

    //get new users from last check (only getting slave because slave never dies)
    //getting new users resets file position with each sync so if another sync joins late,
    //they will not have all users
    std::vector<std::string> newUsers = getNewUsersFromFile("slave_myusers.txt");
    //send new users to all other followsyncs
    for (int i = 1; i <= 3; i++){
        if (!fsyncs[i].ip.empty() && i != stoi(id)){
          //send new users
          syncReq request;
          syncRep reply;            
          grpc::ClientContext syncContext;
          request.set_username(id);
          for(std::vector<std::string>::iterator it = newUsers.begin(); it != newUsers.end(); ++it) {
            request.add_arguments(*it);
          }

          grpc::Status status = fsyncs[i].sync_stub_->newUsers(&syncContext,request,&reply);
          
          if (status.ok()) {
              std::cout<<"sent new users to fsync "<<i<<" "<<std::endl;
          } else {
              std::cout<<"could not send new users to fsync "<<i<<" "<<std::endl;
          }
        }

    }

        //send new followings to followings fsyncs
    //following fsync needs to add these users to its followers file.

    //get all of my users
    std::vector<std::string> users = getUsersFromFile("slave_myusers.txt");
    //iterate through each my user
    for(std::vector<std::string>::iterator it = users.begin(); it != users.end(); ++it) {
      std::string userFollowingFile = "slave_"+*it+"_following.txt";
      std::vector<f_user> newFollowing = getNewFollowsFromFile(userFollowingFile);            
           
      //for each user, get new followings
      for(std::vector<f_user>::iterator j = newFollowing.begin(); j != newFollowing.end(); ++j) {
        //need to redeclare context with every stub function;
        grpc::ClientContext syncContext;
        syncReq request;
        syncRep reply; 
         //username is follower
        request.set_username(*it);
        //send user to     
        //argument is following
        request.add_arguments(j->username);
        
        //int fsync_id = (stoi(j->username)%3)+1;
        int fsync_id = getFsyncId(stoi(j->username));
        grpc::Status fsyncStatus = fsyncs[fsync_id].sync_stub_->newFollow(&syncContext,request,&reply);
        if (fsyncStatus.ok()) {
              std::cout<<"sent new follower "<<*it<<" to followed "<<j->username<<" in fsync "<<fsync_id<<" "<<std::endl;
          } else {
              std::cout<<"could not send new follower "<<*it<<" to followed "<<j->username<<" in fsync "<<fsync_id<<" "<<std::endl;
          }
      }
    }

    //for every one of myusers, check if there are any new updates to their mytimelines
    //send these new posts to each one of their followers fsyncs
    //these follower fsyncs must add these posts to users' othertimeline file

    for(std::vector<std::string>::iterator it = users.begin(); it != users.end(); ++it) {
      std::string userFollowersFile = "slave_"+*it+"_followers.txt";
      std::string userPostsFile = "slave_"+*it+"_mytimeline.txt";
      std::vector<std::string> followers = getUsersFromFile(userFollowersFile);
      std::vector<Message> newPosts = getNewPostsFromFile(userPostsFile); 
      for(std::vector<std::string>::iterator f = followers.begin(); f != followers.end(); ++f) {
        for(std::vector<Message>::iterator p = newPosts.begin(); p != newPosts.end(); ++p) {
          grpc::ClientContext syncContext;
          syncRep reply; 
          Message message = *p;
          message.set_recipient(*f);
          //int fsync_id = (stoi(*f)%3)+1;
          int fsync_id = getFsyncId(stoi(*f));
          grpc::Status fsyncStatus = fsyncs[fsync_id].sync_stub_->newPost(&syncContext,message,&reply);
          if (fsyncStatus.ok()) {
                std::cout<<"sent new post from "<<*it<<" ("<<p->msg()<<")"<<" to follower "<<*f<<" in fsync "<<fsync_id<<" "<<std::endl;
            } else {
                std::cout<<"could not sent new post from "<<*it<<" ("<<p->msg()<<")"<<" to follower "<<*f<<" in fsync "<<fsync_id<<" "<<std::endl;
            }



        }
      }

      

    }
    
}

class SNSSynchronizerImpl final : public SNSSynchronizer::Service {

    Status newFollow(ServerContext* context, const syncReq* request, syncRep* reply) override {
      std::string newFollower = request->username();
      std::string following = request->arguments(0);
      addUsertoFile(newFollower,"slave_"+following+"_followers.txt");
      addUsertoFile(newFollower,"master_"+following+"_followers.txt");

      return Status::OK;

    }
    Status newUsers(ServerContext* context, const syncReq* request, syncRep* reply) override {
         for (int i = 0; i < request->arguments_size();i++ ){
           addUsertoFile(request->arguments(i),"slave_otherusers.txt");
           addUsertoFile(request->arguments(i),"master_otherusers.txt");
          }
        return Status::OK;

    }
    Status newPost(ServerContext* context, const Message* message, syncRep* reply) override {
        std::string recipient = message->recipient();
        addPostToFile(*message,"slave_"+recipient+"_othertimeline.txt");
        addPostToFile(*message,"master_"+recipient+"_othertimeline.txt");
        return Status::OK;

    }
};


void RunServer(std::string port_no) {

  std::string server_address = "localhost:"+port_no;
  SNSSynchronizerImpl service;

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
  port = "9790";
  id = "1";
  int opt = 0;
  while ((opt = getopt(argc, argv, "c:p:m:i:")) != -1){
    switch(opt) {
        case 'c':
          coordinatorIP = optarg;break;
        case 'p':
          coordinatorPort = optarg;break;
      case 'm':
          port = optarg;break;
        case 'i':
          id = optarg;break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  std::string output = "logfiles/synchronizer"+id+"_output.txt";
  freopen(output.c_str(),"w",stdout);
  //prefix is used everywhere to ensure 
  prefix = prefix+"s"+id+"_";

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
    heartbeat.set_s_type(SYNCHRONIZER);
    heartbeat.set_ip("localhost");
    heartbeat.set_port(port);
    std::thread writer([stream,heartbeat]() {
       std::time_t begin = std::time(0);  
       while(1){       
        stream->Write(heartbeat);
        std::this_thread::sleep_for(std::chrono::seconds(10)); 
       }
        
    });

    std::thread synchronize([]() {
       std::time_t begin = std::time(0);  
       while(1){       
       //stream->Write(heartbeat);
        std::cout<<"performing sync"<<std::endl;
        performSync();
        std::this_thread::sleep_for(std::chrono::seconds(30)); 
       }
        
    });

    RunServer(port);

    synchronize.join();
    writer.join();


  return 0;
}