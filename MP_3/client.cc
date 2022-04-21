#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <sstream> 
#include <string>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include "client.h"

#include "coord.grpc.pb.h"
#include "service.grpc.pb.h"

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
// using csce438::Message;
// using csce438::ListReply;
// using csce438::Request;
// using csce438::Reply;
// using csce438::SNSService;

using service::Message;
using service::Request;
using service::Reply;
using service::SNSServer;
using google::protobuf::util::TimeUtil;

std::atomic<bool> exit_thread_flag{false};

bool getline_async(std::istream& is, std::string& str, char delim = '\n') {

    static std::string lineSoFar;
    char inChar;
    int charsRead = 0;
    bool lineRead = false;
    str = "";

    do {
        charsRead = is.readsome(&inChar, 1);
        if (charsRead == 1) {
            // if the delimiter is read then return the string so far
            if (inChar == delim) {
                str = lineSoFar;
                lineSoFar = "";
                lineRead = true;
            } else {  // otherwise add it to the string so far
                lineSoFar.append(1, inChar);
            }
        }
    } while (charsRead != 0 && !lineRead);

    return lineRead;
}

class Client : public IClient
{
    public:
        Client(const std::string& coordIP,
               const std::string& coordPort,
               const std::string& clientId)
            :coordinatorIP(coordIP), coordinatorPort(coordPort), id(clientId)
            {}
    protected:
        virtual int connectTo();
        virtual IReply processCommand(std::string& input);
        virtual void processTimeline();
    private:
        std::string coordinatorIP;
        std::string coordinatorPort;
        std::string id;
        // You can have an instance of the client stub
        // as a member variable.
        std::unique_ptr<SNSCoordinator::Stub> stub_;
        std::unique_ptr<SNSServer::Stub> server_stub_;

        //IReply Login();
        IReply List();
        IReply Follow(const std::string& username2);
        void Timeline(const std::string& username);


};

int main(int argc, char** argv) {

    std::string coordIP = "localhost";
    std::string coordPort = "3010";
    std::string clientId ="0" ;
    int opt = 0;
    while ((opt = getopt(argc, argv, "c:p:i:")) != -1){
        switch(opt) {
            case 'c':
                coordIP = optarg;break;
            case 'p':
                coordPort = optarg;break;
            case 'i':
                clientId = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    Client myc(coordIP, coordPort, clientId);
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

    //connect to coordinator
	std::stringstream ss;
	ss << coordinatorIP << ':' << coordinatorPort;
	std::string s = ss.str();
    auto channel = grpc::CreateChannel(s, grpc::InsecureChannelCredentials());
    stub_ = coord::SNSCoordinator::NewStub(channel);
    
    IReply ire;
    coord::ClientReply reply;
    coord::ClientRequest login_req;
    grpc::ClientContext context;
    login_req.set_id(stoi(id));
    std::string type;
    while (true){        
        grpc::Status status = stub_->ClientLogin(&context,login_req,&reply);
        ire.grpc_status = status;
        if (status.ok()) {
            ire.comm_status = SUCCESS;
            
            if (reply.type() == MASTER){
                type = "MASTER";
            }
            else{
                type = "SLAVE";
            }
            
            break;
        } else {
            ire.comm_status = FAILURE_NOT_EXISTS;
            std::cout<<"not ok";
        }
    }

    //connect to master/slave
    std::string serverLoc = reply.ip()+":"+reply.port();
    auto servchannel = grpc::CreateChannel(serverLoc, grpc::InsecureChannelCredentials());
    server_stub_ = service::SNSServer::NewStub(servchannel);

    Request serverReq;
    Reply serverRep;
    grpc::ClientContext servcontext;
    serverReq.set_username(id);

    grpc::Status serverStatus = server_stub_->Login(&servcontext,serverReq,&serverRep);
    ire.grpc_status = serverStatus;
    if (serverStatus.ok()) {
        ire.comm_status = SUCCESS;
        //std::cout<<"server ok"<<serverRep.msg()<<std::endl;
        std::cout<<"Connected to "<<type<<" server at "<<reply.ip()<<":"<<reply.port()<<std::endl;
        return 1;
    } else {
        ire.comm_status = FAILURE_NOT_EXISTS;
        std::cout<<"server not ok";
        return -1;
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
	// - JOIN/LEAVE and "<username>" are separated by one space.
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
    // Suppose you have "Join" service method for JOIN command,
    // IReply can be set as follow:
    // 
    //     // some codes for creating/initializing parameters for
    //     // service method
    //     IReply ire;
    //     grpc::Status status = stub_->Join(&context, /* some parameters */);
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
    IReply ire;
    std::size_t index = input.find_first_of(" ");
    if (index != std::string::npos) {
        std::string cmd = input.substr(0, index);


        /*
        if (input.length() == index + 1) {
            std::cout << "Invalid Input -- No Arguments Given\n";
        }
        */

        std::string argument = input.substr(index+1, (input.length()-index));

        if (cmd == "FOLLOW") {
            return Follow(argument);
        } 
    } else {
        if (input == "LIST") {
            return List();
        } else if (input == "TIMELINE") {
            ire.comm_status = SUCCESS;
            return ire;
        }
    }

    ire.comm_status = FAILURE_INVALID;
    return ire;
}

void Client::processTimeline()
{
    Timeline(id);
}

// IReply Client::Login() {
//     Request request;
//     request.set_username(username);
//     Reply reply;
//     ClientContext context;

//     Status status = stub_->Login(&context, request, &reply);

//     IReply ire;
//     ire.grpc_status = status;
//     if (reply.msg() == "you have already joined") {
//         ire.comm_status = FAILURE_ALREADY_EXISTS;
//     } else {
//         ire.comm_status = SUCCESS;
//     }
//     return ire;
// }
IReply Client::List() {
    IReply ire;
    while(1){
        
        Reply reply;
        Request list_req;
        grpc::ClientContext context;
        list_req.set_username(id);
        grpc::Status status = server_stub_->List(&context,list_req,&reply);
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
            } break;
        }else {
            if (!connectTo()){
                ire.comm_status = FAILURE_UNKNOWN;
                std::cout<<"not ok";
            }
        }
    }
    return ire;
}
IReply Client::Follow(const std::string& username2){


    IReply ire;
    while (1){
        Reply reply;
        Request follow_req;
        grpc::ClientContext context;
        follow_req.set_username(id);
        follow_req.add_arguments(username2);
        grpc::Status status = server_stub_->Follow(&context,follow_req,&reply);
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
            break;
        } else {
            if (!connectTo()){
                ire.comm_status = FAILURE_UNKNOWN;
                std::cout<<"not ok";
            }
        }
    }
    return ire;
}

void Client::Timeline(const std::string& username) {
    while (1){
        grpc::ClientContext context;

        std::shared_ptr<grpc::ClientReaderWriter<Message, Message> > stream(
        server_stub_->Timeline(&context));
        
        std::thread reader([stream,this]() {
            
            //while (true){
                Message server_note;
                while (stream->Read(&server_note)) {
                //std::cout << "Got message " << server_note.msg() << std::endl;
                auto Time = TimeUtil::TimestampToTimeT(server_note.timestamp());
                displayPostMessage(server_note.username(),server_note.msg(),Time);
                }
                exit_thread_flag = true;
                //std::cout<<"stream read ended"<<std::endl;
                //break;
            //}
            
        });
        //writer.join();
        
        Message joinmsg;
        joinmsg.set_username(username);
        stream->Write(joinmsg);
            while (!exit_thread_flag){
                Message message;
                std::string s1;
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(0,&fds);
                struct timeval tv = {0,50000};   
                if (select(1, &fds, 0, 0,&tv)==0){
                    continue;
                }
                getline(std::cin,s1);
                message.set_username(username);
                message.set_msg(s1);
                auto timestamp = new google::protobuf::Timestamp{};
            timestamp->set_seconds(time(NULL));
            timestamp->set_nanos(0);
            message.set_allocated_timestamp(timestamp);
                stream->Write(message);
                
                
            }
        reader.join();
        if (!connectTo()){
            std::cout<<"slave not found";
        }
        exit_thread_flag = false;
    }



}



