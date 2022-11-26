#include <string>
#include <vector>
#include <set>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <cmath>
#include <sys/file.h>
#include <fcntl.h>
#include <map>
// #include <openssl/sha.h>
#define BUF_SIZE 10000

using namespace std;

map<string,set<string> > file2user;
map<string,set<string> > stop_share_users; // file to user that stopped files
map<string,set<string> > user2files;

struct File{
    unsigned int size;
    string hash;
    unsigned int numChunks;
    vector<string> hashChunks;
};

map<string, File> fileinfo;

struct Peer{
    string name;
    string pass;
    bool isLogged;
    string ip;
    string port;
};

struct Group
{
    int num;
    set<string> users;
    string admin_id;
    set<string> filenames;
    set<string> requests;
};

map<string,Peer> userinfo;
map<string,Group> groupinfo;




int getFileSize(string fname){
    ifstream file(fname, ios::binary | ios::ate );
    return file.tellg();
}
void parseCommand(const string &command,vector<string> &tokens){
    stringstream ss(command);
    string temp;
    while (getline (ss, temp, ' ')) {
        tokens.push_back (temp);
    }
}

string menu(char buf[],string &user, map<string,Peer>& peers){
    vector<string> commands;
    string s = buf;
    parseCommand(s,commands);
    int n = commands.size();
    if(commands[0] == "create_user"){
        string username;
        string password;
        if(n != 3){
            return "correct way: create_user <username> <password>";
        }else if(userinfo.find(commands[1]) != userinfo.end()){
            return "username exists, try again with different username\n";
        }else{
            username = commands[1];
            password = commands[2];
            userinfo[username].pass = peers[username].pass = password;
            userinfo[username].isLogged = peers[username].isLogged = false;
            return "username created sucessfully\n";
        }
    }
    else if(commands[0] == "login"){
        if(n != 3){
            return "correct way: login <username> <password>\n";
        }
        string username = commands[1];
        string password = commands[2];
        if(userinfo.find(username) == userinfo.end()){
            return "Username doest exist\n";
        }
        if(userinfo[username].pass != password){
            return "Wrong Password\n";
        }
        user = username;
        userinfo[user].isLogged = true;
        set<string> files = user2files[user];
            for(auto file :files){
                cout << file << endl;
                stop_share_users[file].erase(user);
            }
        return "Logged in as "+user;

    }else if(commands[0] == "create_group"){
        if(user != "" && userinfo[user].isLogged){
            if(n != 2){
                return "correct way: create_group <group_id>\n";
            }else{
                string id = commands[1];
                if(groupinfo.find(id) != groupinfo.end()){
                    return "group id already exists\n";
                }else{
                    groupinfo[id].admin_id = user;
                    groupinfo[id].num = 1;
                    groupinfo[id].users.insert(user);
                    return "Group "+id+" Added Successfully\n";
                }
            }
        }else{
            return "Login First\n";
        }

    }else if(commands[0] == "join_group"){
        if(user != "" && userinfo[user].isLogged){
            if(n != 2){
                return "correct way: join_group <group_id>\n";
            }else{
                string id = commands[1];
                if(groupinfo.find(id) == groupinfo.end()){
                    return "group does not exists\n";
                }
                else if(groupinfo[id].users.find(user) != groupinfo[id].users.end()){
                    return "group already joined\n";
                }else{
                    // groupinfo[id].num++;
                    groupinfo[id].requests.insert(user);
                    return "Group "+id+" request send\n";
                }
            }
        }else{
            return "Login First\n";
        }

    }else if(commands[0] == "leave_group"){
        if(user != "" && userinfo[user].isLogged){
            if(n != 2){
                return "correct way: leave_group <group_id>\n";
            }else{
                string id = commands[1];
                if(groupinfo.find(id) == groupinfo.end()){
                    return "group does not exists\n";
                }
                else if(groupinfo[id].users.find(user) == groupinfo[id].users.end()){
                    return "Not part of the group already\n";
                }else{
                    if(groupinfo[id].admin_id == user){
                        groupinfo.erase(id);
                        return "You were the admin of the group and deleted the group successfully\n";
                    }else{
                        groupinfo[id].num--;
                        groupinfo[id].users.erase(user);
                        return "Group "+id+" left Successfully\n";
                    }
                    
                }
            }
        }else{
            return "Login First\n";
        }

    }else if(commands[0] == "requests" && commands[1] == "list_requests"){
        if(user != "" && userinfo[user].isLogged){
            if(n != 3){
                return "correct way: requests list_requests <group_id>\n";
            }else{
                string id = commands[2];
                if(groupinfo.find(id) == groupinfo.end()){
                    return "group does not exists\n";
                }
                if(groupinfo[id].admin_id != user){
                    return "You need to be Admin of this group to see requests\n";
                }
                else if(groupinfo[id].requests.size() == 0){
                    return "No requests in queue\n";
                }else{
                    string ret = "User Requests:\n";
                    for(auto i : groupinfo[id].requests){
                        ret += i +'\n';
                    }
                    return ret;
                    
                }
            }
        }else{
            return "Login First\n";
        }

    }else if(commands[0] == "accept_request"){
        if(user != "" && userinfo[user].isLogged){
            if(n != 3){
                return "correct way: requests <group_id> <user_id>\n";
            }else{
                string id = commands[1];
                string uid = commands[2];
                if(groupinfo.find(id) == groupinfo.end()){
                    return "group does not exists\n";
                }
                if(groupinfo[id].admin_id != user){
                    return "You need to be Admin of this group to accept requests\n";
                }
                else if(groupinfo[id].requests.size() == 0){
                    return "No requests in queue\n";
                }
                else if(groupinfo[id].requests.find(uid) != groupinfo[id].requests.end()){
                    groupinfo[id].users.insert(uid);
                    groupinfo[id].num++;
                    groupinfo[id].requests.erase(uid); 
                    return "User "+ uid+" inserted in Group "+id+"\n";                   
                }else{
                    return "user id not found in requests\n";
                }
            }
        }else{
            return "Login First\n";
        }

    }else if(commands[0] == "list_groups"){
        if(user != "" && userinfo[user].isLogged){
            if(n != 1){
                return "correct way: list_groups\n";
            }else{
                string ret = "Groups in Network:\n";
                for(auto i : groupinfo){
                    ret += i.first+"\n";
                }
                return ret;
            }
        }else{
            return "Login First\n";
        }

    }else if(commands[0] == "list_files"){
        if(user != "" && userinfo[user].isLogged){
            if(n != 2){
                return "correct way: list_files <group_id>\n";
            }else{
                string id = commands[1];
                if(groupinfo.find(id) == groupinfo.end()){
                    return "group does not exists\n";
                }
                else if(groupinfo[id].users.find(user) == groupinfo[id].users.end()){
                    return "User Not part of this group\n";
                }
                else if(groupinfo[id].filenames.size() == 0){
                    return "No Files shared in this group\n";
                }else{
                    string ret = "Files in this group:\n";
                    for(auto i : groupinfo[id].filenames){
                        ret += i+"\n";
                    }
                    return ret;
                }
            }
        }else{
            return "Login First\n";
        }
    //custom command only for client tracker
    }else if(commands[0] == "up"){
        if(user != "" && userinfo[user].isLogged){
            cout << s << endl;
            string id = commands[2];
            string fname = commands[1];
            string u_ip = commands[3];
            string u_port = commands[4];
            string size = commands[5];
            string hash = commands[6];
            if(groupinfo.find(id) == groupinfo.end()){
                return "group does not exists\n";
            }else if(groupinfo[id].users.find(user) == groupinfo[id].users.end()){
                return "User Not part of this group\n";
            }
            else if(groupinfo[id].filenames.find(fname) != groupinfo[id].filenames.end()){
                return "File already uploaded\n";
            }
            else{
                groupinfo[id].filenames.insert(fname);
                userinfo[user].ip = u_ip;
                user2files[user].insert(fname);
                userinfo[user].port = u_port;
                file2user[fname].insert(user);
                fileinfo[fname].hash = hash;
                fileinfo[fname].numChunks = n - 7;
                fileinfo[fname].size = stoi(size);
                for(int i = 7 ; i < n ;i++){
                    fileinfo[fname].hashChunks.push_back(commands[i]);
                    cout << commands[i] <<endl;
                }
                return fname+" Successfully uploaded\n";
            }
    }else{
            return "Login First\n";
        }

    
    }else if(commands[0] == "down"){
        if(user != "" && userinfo[user].isLogged){
            if(n != 5){
                return "correct way: down <group_id> <file_name>\n";
            }else{
                string id = commands[1];
                string fname = commands[2];
                string u_ip = commands[3];
                string u_port = commands[4];
                set<string> users = file2user[fname];
                if(groupinfo.find(id) == groupinfo.end()){
                    return "group does not exists\n";
                }
                else if(groupinfo[id].users.find(user) == groupinfo[id].users.end()){
                    return "User Not part of this group\n";
                }else if(groupinfo[id].filenames.find(fname) == groupinfo[id].filenames.end()){
                    return "File Info not found\n";
                }else{
                    int flag = 1;
                    string res = "download " + fname + " " + to_string(fileinfo[fname].size);
                    for(auto x: users){
                        if(x != user && stop_share_users[fname].find(x) == stop_share_users[fname].end()){
                            res = res +" "+ userinfo[x].ip + " "+userinfo[x].port;
                            flag = 0;
                        }
                    }
                    res = res + " file_hash " + fileinfo[fname].hash;
                    for(auto x: fileinfo[fname].hashChunks){
                        res = res + " " + x;
                    }
                    cout << res << endl;
                    if(flag){
                        return "No Peers found!!!\n";
                    }
                    file2user[fname].insert(user);
                    userinfo[user].ip = u_ip;
                    userinfo[user].port = u_port;
                    return res;
                }
            }
        }else{
            return "Login First\n";
        }
    }else if(commands[0] == "logout"){
        if(userinfo[user].isLogged){
            userinfo[user].isLogged = false;
            set<string> files = user2files[user];
            for(auto file :files){
                stop_share_users[file].insert(user);
            }
            user = "";

            return "Logged out";
        }
        return "Already logged out";
    }else if(commands[0] == "show_downloads"){
        if(user != "" && userinfo[user].isLogged){
            
        }else{
            return "Login First\n";
        }
    }else if(commands[0] == "stop_share"){
        if(user != "" && userinfo[user].isLogged){
            if(n != 3){
                return "correct way: stop_share <group_id> <file_name>\n";
            }else{
                string id = commands[1];
                string fname = commands[2];
                if(groupinfo.find(id) == groupinfo.end()){
                    return "group does not exists\n";
                }
                else if(groupinfo[id].users.find(user) == groupinfo[id].users.end()){
                    return "User Not part of this group\n";
                }else if(groupinfo[id].filenames.find(fname) == groupinfo[id].filenames.end()){
                    return "File Info not found\n";
                }else{
                    stop_share_users[fname].insert(user);
                    return "Stopped Sharing File " + fname;
                }
            }
        }else{
            return "Login First\n";
        }
    }else{
        return "No Commands Found";
    }
    
}



// Runs in a thread
void* listen_client(void* cl){
    //sockid descriptor to read and write data from

    int client = *(int*)cl;
    char buf[10000];
    string user;
    map<string,Peer> peer;


    while(1){
        memset(&buf,0,sizeof(buf));
        int resp = recv(client,(char*)&buf,sizeof(buf),0);
        if(resp <= 0){
            cout << resp <<endl;
            break;
        }
        string res = menu(buf,user,peer);
        memset(&buf,0,sizeof(buf));
        strcpy(buf, res.c_str());
        write(client,buf,sizeof(buf));
    }
    return NULL;
}

int main(int argc, char const *argv[])
{
    ifstream file(argv[1]);
    string address, port;
    getline(file,address);
    getline(file,port);
    const char * addr = address.c_str();
    int pt = atoi(port.c_str());
    int sockfd;
    socklen_t len;
    struct sockaddr_in servaddr, cli;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        cout << "Socket cannot be created" << endl;
        exit(0);
    }
    memset((char*)&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(pt);
    if (::bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        cout << "Socket cannot be binded" << endl;
        exit(0);
    }
    if ((listen(sockfd, 5)) != 0)
    {
        cout << "Server cannot listen" << endl;
        exit(0);
    }
    else
        cout << "Server Listening for clients on " << addr << " " << pt << endl;

    len = sizeof(cli);
    for(;;)
    {
        int connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
        int *clientfd = (int *)malloc(sizeof(int));
        *clientfd = connfd;
        if (connfd < 0)
        {
            cout << "Server Cannot open socket" << endl;
            exit(0);
        }
        cout << "Client Connected" << endl;
        
        pthread_t th;
        pthread_create(&th,NULL,listen_client,clientfd);
    }
    
    close(sockfd);
    return 0;
}


