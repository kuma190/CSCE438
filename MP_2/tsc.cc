#include <iostream>
#include <string>
#include <unistd.h>
#include <sstream> 
#include <grpc++/grpc++.h>
#include "client.h"
#include "sns.grpc.pb.h"
#include <google/protobuf/util/time_util.h>
#include <ctime>
#include <thread>

using grpc::Status;
using csce438::Message;
using csce438::Request;
using csce438::Reply;
using csce438::SNSService;
using namespace google::protobuf::util;

// using google::protobuf::Timestamp;
// using google::protobuf::Duration;

class Client : public IClient
{
    public:
        Client(const std::string& hname,
               const std::string& uname,
               const std::string& p)
            :hostname(hname), username(uname), port(p)
            {}
    protected:
        virtual int connectTo();
        virtual IReply processCommand(std::string& input);
        virtual void processTimeline();
    private:
        std::string hostname;
        std::string username;
        std::string port;
        //auto channel;
        
        // You can have an instance of the client stub
        // as a member variable.
        std::unique_ptr<csce438::SNSService::Stub> stub_;
        //std::unique_ptr stub_;
};

int main(int argc, char** argv) {

    std::string hostname = "localhost";
    std::string username = "default";
    std::string port = "3010";
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:u:p:")) != -1){
        switch(opt) {
            case 'h':
                hostname = optarg;break;
            case 'u':
                username = optarg;break;
            case 'p':
                port = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    Client myc(hostname, username, port);
    // You MUST invoke "run_client" function to start business logic
    myc.run_client();

    return 0;
}

int Client::connectTo()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to create a stub so that
    // you call service methods in the processCommand/porcessTimeline
    // functions. That is, the stub should be accessible when you want
    // to call any service methods in those functions.
    // I recommend you to have the stub as
    // a member variable in your own Client class.
    // Please refer to gRpc tutorial how to create a stub.
	// ------------------------------------------------------------
	std::stringstream ss;
	ss << hostname << ':' << port;
	std::string s = ss.str();
    auto channel = grpc::CreateChannel(s, grpc::InsecureChannelCredentials());
    stub_ = csce438::SNSService::NewStub(channel);
    
    IReply ire;
    csce438::Reply reply;
    csce438::Request login_req;
    grpc::ClientContext context;
    login_req.set_username(username);
    
    grpc::Status status = stub_->Login(&context,login_req,&reply);
        ire.grpc_status = status;
        if (status.ok()) {
            ire.comm_status = SUCCESS;
            //std::cout<<"ok"<<reply.msg();
        } else {
            ire.comm_status = FAILURE_NOT_EXISTS;
            //std::cout<<"not ok";
        }
    
    //return ire;
    
    
    return 1; // return 1 if success, otherwise return -1
}

IReply Client::processCommand(std::string& input)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse the given input
    // command and create your own message so that you call an 
    // appropriate service method. The input command will be one
    // of the followings:
	//
	// FOLLOW <username>
	// UNFOLLOW <username>
	// LIST
    // TIMELINE
	//
	// ------------------------------------------------------------
	
    // ------------------------------------------------------------
	// GUIDE 2:
	// Then, you should create a variable of IReply structure
	// provided by the client.h and initialize it according to
	// the result. Finally you can finish this function by returning
    // the IReply.
	// ------------------------------------------------------------
    
	// ------------------------------------------------------------
    // HINT: How to set the IReply?
    // Suppose you have "Follow" service method for FOLLOW command,
    // IReply can be set as follow:
    // 
    //     // some codes for creating/initializing parameters for
    //     // service method
    //     IReply ire;
    //     grpc::Status status = stub_->Follow(&context, /* some parameters */);
    //     ire.grpc_status = status;
    //     if (status.ok()) {
    //         ire.comm_status = SUCCESS;
    //     } else {
    //         ire.comm_status = FAILURE_NOT_EXISTS;
    //     }
    //      
    //      return ire;
    // 
    // IMPORTANT: 
    // For the command "LIST", you should set both "all_users" and 
    // "following_users" member variable of IReply.
    // ------------------------------------------------------------
    std::string command;
    std::string argument;
    std::string delimiter;
    if (input != "LIST" && input != "TIMELINE") {
        delimiter = " ";
        command = input.substr(0, input.find(delimiter));
        argument = input.substr(input.find(delimiter)+1,input.length());
    }
    else{
        command = input;
    }
    IReply ire;
    if (command == "FOLLOW"){
        csce438::Reply reply;
        csce438::Request follow_req;
        grpc::ClientContext context;
        follow_req.set_username(username);
        follow_req.add_arguments(argument);
        grpc::Status status = stub_->Follow(&context,follow_req,&reply);
        ire.grpc_status = status;
        if (status.ok()) {
            if (reply.msg() == "1"){
                ire.comm_status=SUCCESS;
            }
            else if (reply.msg() == "2"){
                ire.comm_status=FAILURE_ALREADY_EXISTS;
            }
            else if (reply.msg() == "4"){
                ire.comm_status= FAILURE_INVALID_USERNAME;
            }
            //std::cout<<"ok"<<reply.msg();
        } else {
            ire.comm_status = FAILURE_UNKNOWN;
            //std::cout<<"not ok";
        }
        return ire;
        
        
        
    }
    else if (command == "UNFOLLOW"){
        csce438::Reply reply;
        csce438::Request unfollow_req;
        grpc::ClientContext context;
        unfollow_req.set_username(username);
        unfollow_req.add_arguments(argument);
        grpc::Status status = stub_->UnFollow(&context,unfollow_req,&reply);
        ire.grpc_status = status;
        if (status.ok()) {
            if (reply.msg() == "1"){
                ire.comm_status=SUCCESS;
            }
            else if (reply.msg() == "3"){
                ire.comm_status=FAILURE_NOT_EXISTS;
            }
            else if (reply.msg() == "4"){
                ire.comm_status= FAILURE_INVALID_USERNAME;
            }
            //std::cout<<"ok"<<reply.msg();
        } else {
            ire.comm_status = FAILURE_UNKNOWN;
            //std::cout<<"not ok";
        }
        return ire;
        
    }
    else if (command == "LIST"){
        csce438::Reply reply;
        csce438::Request list_req;
        grpc::ClientContext context;
        list_req.set_username(username);
        grpc::Status status = stub_->List(&context,list_req,&reply);
        ire.grpc_status = status;
        if (status.ok()){
            if (reply.msg()== "1"){
                ire.comm_status=SUCCESS;
                for (int i = 0; i < reply.all_users_size();i++ ){
                    ire.all_users.push_back(reply.all_users(i));
                }
                for (int i = 0; i < reply.following_users_size();i++ ){
                    ire.following_users.push_back(reply.following_users(i));
                }
            }
            else{
                ire.comm_status= FAILURE_INVALID_USERNAME;
            }
        }else {
            ire.comm_status = FAILURE_UNKNOWN;
            //std::cout<<"not ok";
        }
        return ire;
        
        
    }
    
    else if (command == "TIMELINE"){
        ire.grpc_status = grpc::Status::OK;
        ire.comm_status = SUCCESS;
        return ire;
        
    }
         
    
    // IReply ire;
    // csce438::Reply reply;
    // csce438::Request login_req;
    // grpc::ClientContext context;
    // login_req.set_username("testuser");
    
    
    
    // grpc::Status status = stub_->Login(&context,login_req,&reply);
    //     ire.grpc_status = status;
    //     if (status.ok()) {
    //         ire.comm_status = SUCCESS;
    //         std::cout<<"ok"<<reply.msg();
    //     } else {
    //         ire.comm_status = FAILURE_NOT_EXISTS;
    //         std::cout<<"not ok";
    //     }
    
    // return ire;
}

void Client::processTimeline()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to get into timeline mode.
    // You may need to call a service method to communicate with
    // the server. Use getPostMessage/displayPostMessage functions
    // for both getting and displaying messages in timeline mode.
    // You should use them as you did in hw1.
	// ------------------------------------------------------------

    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    //
    // Once a user enter to timeline mode , there is no way
    // to command mode. You don't have to worry about this situation,
    // and you can terminate the client program by pressing
    // CTRL-C (SIGINT)
	// ------------------------------------------------------------
	grpc::ClientContext context;

    std::shared_ptr<grpc::ClientReaderWriter<Message, Message> > stream(
    stub_->Timeline(&context));
    
    std::thread reader([stream,this]() {
        
        while (true){
            Message server_note;
            while (stream->Read(&server_note)) {
              //std::cout << "Got message " << server_note.msg() << std::endl;
              auto Time = TimeUtil::TimestampToTimeT(server_note.timestamp());
              displayPostMessage(server_note.username(),server_note.msg(),Time);
            }
        }
        
    });
    //writer.join();
    
    //std::thread writer([stream,this]() {
    Message joinmsg;
    joinmsg.set_username(username);
    stream->Write(joinmsg);
        while (true){
            Message message;
            std::string s1;
            getline(std::cin,s1);
            message.set_username(username);
            message.set_msg(s1);
            auto timestamp = new google::protobuf::Timestamp{};
          timestamp->set_seconds(time(NULL));
          timestamp->set_nanos(0);
          message.set_allocated_timestamp(timestamp);
            stream->Write(message);
            
            
        }
        // std::vector<RouteNote> notes{MakeRouteNote("First message", 0, 0),
        //                           MakeRouteNote("Second message", 0, 1),
        //                           MakeRouteNote("Third message", 1, 0),
        //                           MakeRouteNote("Fourth message", 0, 0)};
        // for (const RouteNote& note : notes) {
        // std::cout << "Sending message " << note.message() << " at "
        //           << note.location().latitude() << ", "
        //           << note.location().longitude() << std::endl;
        // stream->Write(note);
        // }
        // stream->WritesDone();
    //});
    reader.join();
    

    
    //writer.join();
}
