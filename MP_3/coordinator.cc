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

#include "coord.grpc.pb.h"

using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using coord::SNSCoordinator;
using coord::ClientRequest;
using coord::ClientReply;
using coord::HeartBeat;
using coord::MASTER;
using coord::SLAVE;
using coord::SYNCHRONIZER;
using coord::COORDINATOR;
using coord::ServerType;


enum condition {
  ACTIVE,
  INACTIVE

};

struct TableEntry{
  std::string ip;
  std::string port_num;
  condition status;


};

std::unordered_map<int, TableEntry> masters;
std::unordered_map<int, TableEntry> slaves;
std::unordered_map<int, TableEntry> synchronizers;



class SNSCoordinatorImpl final : public SNSCoordinator::Service {

  Status ClientLogin(ServerContext* context, const ClientRequest* request, ClientReply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // a new user and verify if the username is available
    // or already taken
    // ------------------------------------------------------------
    int id = request->id();
    std::string returnString = "got client "+std::to_string(id);
    std::cout<<returnString<<std::endl;
    // std::string userfile = "users.txt";
    // if (doesUserExistInFile(username,userfile) == false){
    //   addUsertoFile(username,userfile);
      
    //   addUserFiles(username);
    //   addUsertoFile(username,username+"_followers.txt");
    //   addUsertoFile(username,username+"_following.txt");
      
    // }
    
    id = (id%3)+1;
    if (masters[id].status == ACTIVE){
      
      reply->set_ip(masters[id].ip);
      reply->set_port(masters[id].port_num);
      reply->set_type(MASTER);

    }
    else if (slaves[id].status == ACTIVE){
      reply->set_ip(slaves[id].ip);
      reply->set_port(slaves[id].port_num);
      reply->set_type(SLAVE);

    }
    else{
      reply->set_ip("NaN");
      reply->set_port("NaN");

    }
    
    return Status::OK;
  }

  Status GetSlave(ServerContext* context, const ClientRequest* request, ClientReply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // a new user and verify if the username is available
    // or already taken
    // ------------------------------------------------------------
    int id = request->id();
    std::string returnString = "got slave request "+std::to_string(id);
    std::cout<<returnString<<std::endl;
    // std::string userfile = "users.txt";
    // if (doesUserExistInFile(username,userfile) == false){
    //   addUsertoFile(username,userfile);
      
    //   addUserFiles(username);
    //   addUsertoFile(username,username+"_followers.txt");
    //   addUsertoFile(username,username+"_following.txt");
      
    // }
    
    //id = (id%3)+1;
    if (slaves[id].status == ACTIVE){
      
      reply->set_ip(slaves[id].ip);
      reply->set_port(slaves[id].port_num);

    }
    else{
      reply->set_ip("NaN");
      reply->set_port("NaN");

    }
    
    return Status::OK;
  }

  Status GetFsync(ServerContext* context, const ClientRequest* request, ClientReply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // a new user and verify if the username is available
    // or already taken
    // ------------------------------------------------------------
    int id = request->id();
    std::string returnString = "got fsync request "+std::to_string(id);
    std::cout<<returnString<<std::endl;
    // std::string userfile = "users.txt";
    // if (doesUserExistInFile(username,userfile) == false){
    //   addUsertoFile(username,userfile);
      
    //   addUserFiles(username);
    //   addUsertoFile(username,username+"_followers.txt");
    //   addUsertoFile(username,username+"_following.txt");
      
    // }
    
    id = (id%3)+1;
    if (synchronizers[id].status == ACTIVE){
      reply->set_id(id);
      reply->set_ip(synchronizers[id].ip);
      reply->set_port(synchronizers[id].port_num);

    }
    else{
      reply->set_ip("NaN");
      reply->set_port("NaN");

    }
    
    return Status::OK;
  }

  Status ServerCommunicate(ServerContext* context, ServerReaderWriter<HeartBeat,HeartBeat>* stream) override {
    HeartBeat note;
    int id;
    ServerType type;

    while (stream->Read(&note)) {
      id = note.sid();
      type = note.s_type();
      if (type == MASTER ){
        std::cout<<"master heartbeat "<<std::to_string(id)<<std::endl;
        if (masters[id].status == INACTIVE){
          masters[id].ip = note.ip();
          masters[id].port_num = note.port();
          masters[id].status = ACTIVE;
        }
      }
      if (type == SLAVE ){
        std::cout<<"slave heartbeat "<<std::to_string(id)<<std::endl;
        if (slaves[id].status == INACTIVE){
          slaves[id].ip = note.ip();
          slaves[id].port_num = note.port();
          slaves[id].status = ACTIVE;
        }
      }
      if (type == SYNCHRONIZER ){
        std::cout<<"synchronizer heartbeat "<<std::to_string(id)<<std::endl;
        if (synchronizers[id].status == INACTIVE){
          synchronizers[id].ip = note.ip();
          synchronizers[id].port_num = note.port();
          synchronizers[id].status = ACTIVE;
        }
      }

    }
    if (type == MASTER ){
      std::cout<<"master ended "<<std::to_string(id)<<std::endl;
      if (masters[id].status == ACTIVE){
        masters[id].status = INACTIVE;
      }
    }
    if (type == SLAVE ){
      std::cout<<"slave ended "<<std::to_string(id)<<std::endl;
      if (slaves[id].status == ACTIVE){
        slaves[id].status = INACTIVE;
      }
    }
    if (type == SYNCHRONIZER ){
      std::cout<<"synchronizer ended "<<std::to_string(id)<<std::endl;
      if (synchronizers[id].status == ACTIVE){
        synchronizers[id].status = INACTIVE;
      }
    }
    return Status::OK;
  }

};

void RunServer(std::string port_no) {
  std::string server_address = "localhost:"+port_no;
  SNSCoordinatorImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char** argv) {
  masters = {
    {1,{"","",INACTIVE}},
    {2,{"","",INACTIVE}},
    {3,{"","",INACTIVE}},

  };

  slaves = {
    {1,{"","",INACTIVE}},
    {2,{"","",INACTIVE}},
    {3,{"","",INACTIVE}},

  };

  synchronizers = {
    {1,{"","",INACTIVE}},
    {2,{"","",INACTIVE}},
    {3,{"","",INACTIVE}},

  };
  
  std::string port = "3010";
  int opt = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1){
    switch(opt) {
      case 'p':
          port = optarg;break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  std::string output = "logfiles/coordinator_output.txt";
  freopen(output.c_str(),"w",stdout);
  RunServer(port);

  return 0;
}