#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>

#include <vector>
#include <algorithm>

#include <stdio.h> 
#include <string.h>   //strlen 
//#include <stdlib.h> 
#include <errno.h> 
//#include <unistd.h>   //close 
#include <arpa/inet.h>    //clos
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <unordered_map>
     
#define TRUE   1 
#define FALSE  0 

#include "sns.grpc.pb.h"

using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using csce438::Message;
using csce438::Request;
using csce438::Reply;
using csce438::SNSService;
using google::protobuf::util::TimeUtil;

std::mutex mu_;
std::unordered_map<std::string, ServerReaderWriter<Message,Message>*> umap;

std::vector<std::string> getUsersFromFile(std::string filename){
  std::string prefix = "userFiles/";
  std::vector<std::string> vect;
  filename = prefix + filename;

  std::ifstream infile;
  infile.open(filename);
  std::string line;
  while (getline(infile, line)){
      //std::cout<<line<<std::endl;
      vect.push_back(line);
  }
  infile.close();
  return vect;
  
}

bool doesUserExistInFile(std::string username,std::string filename){
    std::string prefix = "userFiles/";
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
  std::cout<<"added "<<username<<" to "<<filename<<std::endl;
  std::string prefix = "userFiles/";
  filename = prefix+filename;
  std::string key = username;
  std::ofstream outfile;
  outfile.open(filename,std::ios_base::app);
  outfile << key<< std::endl;
  outfile.close();
}

void addUserFiles(std::string username){
  std::string prefix = "userFiles/";
  std::string key = username;
  std::ofstream outfile;
  outfile.open(prefix+key+"_timeline.txt");
  outfile.close();
  outfile.open(prefix+key+"_followers.txt");
  outfile.close();
  outfile.open(prefix+key+"_following.txt");
  outfile.close();
  std::cout << "User files added"<<std::endl;
  
}

void removeUserFromFile(std::string username,std::string filename){
  std::string prefix = "userFiles/";
  std::vector<std::string> vect;
  vect = getUsersFromFile(filename);
  filename = prefix + filename;

  
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
  std::string prefix = "userFiles/";
  filename = prefix+filename;
  std::ofstream outfile;
  outfile.open(filename,std::ios_base::app);
  outfile << note.username()<< std::endl;
  outfile << note.msg()<< std::endl;
  std::string strStamp = TimeUtil::ToString(note.timestamp());
  outfile << strStamp<< std::endl;
  outfile.close();
  
}

std::vector<Message> getPostsFromFile(std::string filename){
  std::string prefix = "userFiles/";
  std::vector<Message> vect;
  filename = prefix + filename;

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
  return vect;
  
}



class SNSServiceImpl final : public SNSService::Service {
  
  Status List(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // LIST request from the user. Ensure that both the fields
    // all_users & following_users are populated
    // ------------------------------------------------------------
    std::string username = request->username();
    std::string message = "0";
    std::vector<std::string> allUsers;
    std::vector<std::string> followers;
    if (doesUserExistInFile(username,"users.txt")){
      allUsers = getUsersFromFile("users.txt");
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
    std::string username = request->username();
    std::string followed = request->arguments(0);
    std::string message = "0";
    if (doesUserExistInFile(followed,"users.txt") && username != followed){
      if (doesUserExistInFile(username,followed+"_followers.txt")==false && doesUserExistInFile(followed,username+"_following.txt")==false){
        addUsertoFile(username,followed+"_followers.txt");
        addUsertoFile(followed,username+"_following.txt");
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

  Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // request from a user to unfollow one of his/her existing
    // followers
    // ------------------------------------------------------------
    std::string username = request->username();
    std::string followed = request->arguments(0);
    std::string message = "0";
    if (doesUserExistInFile(followed,"users.txt") && username != followed){
      if (doesUserExistInFile(username,followed+"_followers.txt")==true && doesUserExistInFile(followed,username+"_following.txt")==true){
        removeUserFromFile(username,followed+"_followers.txt");
        removeUserFromFile(followed,username+"_following.txt");
        message = "1";
      }
      else{
        message = "3";
      }
    }
    else{
      message = "4";
    }
    
    reply->set_msg(message);
    
    return Status::OK;
  }
  
  Status Login(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // a new user and verify if the username is available
    // or already taken
    // ------------------------------------------------------------
    std::string username = request->username();
    std::string returnString = "got "+username;
    std::string userfile = "users.txt";
    if (doesUserExistInFile(username,userfile) == false){
      addUsertoFile(username,userfile);
      
      addUserFiles(username);
      addUsertoFile(username,username+"_followers.txt");
      
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
    std::cout<<"server start"<<std::endl;
    while (stream->Read(&note)) {
      //std::unique_lock<std::mutex> lock(mu_);
      
      //stream->Write(note);
      username = note.username();
      std::vector<std::string> followers;
      
      if (umap.find(username) == umap.end()){
        umap[username]=stream;
        std::vector<Message> posts = getPostsFromFile(username+"_timeline.txt");
        int count = 0;
        for (std::vector<Message>::reverse_iterator it = posts.rbegin(); it != posts.rend(); it++){
          if (count == 20){
            break;
          }
            stream->Write(*it);
            count++;
        }
        
      }
      else{
      followers = getUsersFromFile(username+"_followers.txt");
      for (std::vector<std::string>::iterator t = followers.begin(); t != followers.end(); ++t){
        std::string element = t->c_str();
        if (element != username){
          addPostToFile(note,element+"_timeline.txt");
          if (umap.find(element) != umap.end()){
            auto newStream = umap[element];
            newStream->Write(note);
          }
        }
      }
      }
        
        
        
        
    }
    if (umap.find(username) != umap.end()){
      umap.erase(username);
    }
    std::cout<<"server end erased "<<username<<std::endl;
    

    return Status::OK;
  }

};


void RunServer(std::string port_no) {
  // ------------------------------------------------------------
  // In this function, you are to write code 
  // which would start the server, make it listen on a particular
  // port number.
  // ------------------------------------------------------------
  std::ifstream userFile;
  userFile.open("userFiles/users.txt");
  if (userFile){
    std::cout<<"userfile exists"<<std::endl;
    userFile.close();
  }
  else{
     std::cout<<"userfile does not exists"<<std::endl;
     std::ofstream outfile;
     outfile.open("userFiles/users.txt");
    outfile.close();
    std::cout<<"userfile created"<<std::endl;
  }
  
  std::string server_address("127.0.0.1:"+port_no);
  SNSServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
  
}

int main(int argc, char** argv) {
  freopen("output.txt","w",stdout);
  std::string port = "3010";
  int opt = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1){
    switch(opt) {
      case 'p':
          port = optarg;
          break;
      default:
	         std::cerr << "Invalid Command Line Argument\n";
    }
  }
  RunServer(port);
  return 0;
}
