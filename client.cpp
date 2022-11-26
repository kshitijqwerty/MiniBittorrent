#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <cmath>
#include <sys/file.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <map>
#include <vector>


#define BUF_SIZE 10000
#define CHUNK_SIZE 512*1000
#define SUB_CHUNK 1024

using namespace std;

string this_ip,this_port;

struct userDetails{
    int sockfd;
    string file_name;
    string path;
    unsigned int size;
    string hash_of_file;
    vector<string> hash_of_chunks;
    int number_of_chunks;
};

struct File{
    unsigned int size;
    string path;
    string hash;
    unsigned int numChunks;
    vector<string> hashChunks;
};

struct peerdetails{
    const char* ip;
    const char* port;
};

struct downFiles{
    int id;
    string status;
};

map<string,File> files;
map<string,downFiles> downloads;

void parseCommand(const string &command,vector<string> &tokens){
    stringstream ss(command);
    string temp;
    while (getline (ss, temp, ' ')) {
        tokens.push_back (temp);
    }
}

int getFileSize(string& fname){
    ifstream file(fname, ios::binary | ios::ate );
    return file.tellg();
}

string getFileName(string& path){
    return path.substr(path.find_last_of("/\\") + 1);
}

void genHashFromBuffer(char* &buff,size_t size,string &out){
    SHA_CTX md;
    unsigned char hash[SHA_DIGEST_LENGTH];
    char outputBuffer[20];
    memset(hash, 0, sizeof(hash));
    SHA1_Init(&md);
    SHA1_Update(&md, buff, size);
    SHA1_Final(hash, &md);
    int i = 0;
    while(i < SHA_DIGEST_LENGTH){
            sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
            i++;
    }
    out = outputBuffer;

}

string genHash(string file){
    vector<string> sha_of_chunk;


    int chunkSize = CHUNK_SIZE;
    ifstream infile;
    infile.open(file);
    char * buffer = (char*)malloc(chunkSize*sizeof(char));
    string out = "";
    infile.seekg(0, ios::end);
    size_t size = infile.tellg();
    unsigned int numChunks = size/chunkSize;
    size_t lastChunkssize = size%chunkSize;
    infile.seekg(0, ios::beg);
    int eof;
    string str = "";
    string hash_of_file = "";
    while(numChunks--){
        memset(buffer,0,chunkSize*sizeof(char));
        infile.read(buffer,chunkSize*sizeof(char));
        genHashFromBuffer(buffer,chunkSize*sizeof(char),out);
        sha_of_chunk.push_back(out);

        int i = 0;
        while(i < 20)
        {
            str += out[i];
            i++;
        }
    }

    memset(buffer, 0, chunkSize*sizeof(char));
    infile.read(buffer,chunkSize*sizeof(char));
    genHashFromBuffer(buffer,chunkSize*sizeof(char),out);
    sha_of_chunk.push_back(out);

    int i = 0;
    while(i < 20)
    {
        str += out[i];
        i++;
    }
    infile.close();
    hash_of_file = str;
    str = "";
    for(int k = 0; k < sha_of_chunk.size();k++){
        str += sha_of_chunk[k] + " ";
    }
    return hash_of_file + " "+str;

}
void* listen_peer(void* cl){
    //sockid descriptor to read and write data from

    int peer = *(int*)cl;
    char buf[SUB_CHUNK];

    while(1){
        memset(&buf,0,sizeof(buf));
        int resp = recv(peer,(char*)&buf,sizeof(buf),0);
        if(resp <= 0){
            break;
        }
        string res;
        vector<string>commands;
        parseCommand(buf,commands);
        if(commands[0] == "sChunk"){
            int chunk = stoi(commands[1]);
            int sub_chunk = stoi(commands[2]);
            ifstream f(files[commands[3]].path.c_str(),ios::binary);
            if(!f.good()){
                res = "NULL";
            }else{
                f.seekg(chunk*CHUNK_SIZE + sub_chunk*SUB_CHUNK,ios::beg);
                char block[SUB_CHUNK];
                f.read(block,SUB_CHUNK);
                string str(block);
                res = str;
            }
            f.close();
        }
        else if(commands[0] == "lChunk"){
            int chunk = stoi(commands[1]);
            int sub_chunk = stoi(commands[2]);
            int lastchunk = stoi(commands[3]);
            ifstream f(files[commands[4]].path.c_str(),ios::binary);
            if(!f.good()){
                res = "NULL";
            }else{
                f.seekg(chunk*CHUNK_SIZE + sub_chunk*SUB_CHUNK,ios::beg);
                char block[lastchunk];
                f.read(block,lastchunk);
                string str(block);
                res = str;
            }
            f.close();
        }else if(commands[0] == "d"){
            string path = files[commands[1]].path;
            res = path;
        }
        memset(&buf,0,sizeof(buf));
        strcpy(buf, res.c_str());
        write(peer,buf,sizeof(buf));
    }
    return NULL;
}

int dfile(string a, string b)
{
	char block[1024];
	int in, out,nread;
	struct stat z;
	if ((stat(a.c_str(), &z) == 0)){
		in = open(a.c_str(), O_RDONLY);
	}
	else{
		return 1;
	}
	if ((stat((b).c_str(), &z) == 0)){
		out = open((b).c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	}
	else{
		return 2;
	}
	while ((nread = read(in, block, sizeof(block))) > 0){
		write(out, block, nread);
	}
	return 3;
}

void *socket_connection(void *p_client_socket)
{
    int si = 0;
    userDetails *peer = (userDetails *)p_client_socket;
    string hash_of_file = peer->hash_of_file;
    vector<string> hChunks = peer->hash_of_chunks;
    int numChunks = peer->number_of_chunks;
    int peerfd = peer->sockfd;
    unsigned int size = peer->size;
    unsigned int lastChunkSize = size%(CHUNK_SIZE);
    string fileName = peer->file_name;
    string path = peer->path;
    string fullPath = path +"/"+fileName;
    ifstream f(fullPath.c_str());
    if (f.good())
    {
        f.close();
    }
    else
    {
        f.close();
        std::ofstream ofs(fullPath, ios::binary | ios::out);
            ofs.seekp(size - 1);
            ofs.write("", 1);
            ofs.close();
    }
    
    fstream outfile(fullPath,ios::binary | ios::out | ios::in);
    for (int i = 0; i < numChunks; i++)
    {
        char buff[SUB_CHUNK];
        memset(&buff, 0, sizeof(buff));
        
        int sub_pieces;
        if(i == numChunks - 1){
            sub_pieces = (lastChunkSize)/(SUB_CHUNK);
        }else{
            sub_pieces = (CHUNK_SIZE)/(SUB_CHUNK);
        }
        string s;
        for(int k = 0; k < sub_pieces;k++){
            s = "sChunk " + to_string(i) + " "+to_string(k)+" "+ fileName;
            if(i == numChunks - 1 && k == sub_pieces - 1){
                s = "lChunk " + to_string(i) + " "+to_string(k)+" "+to_string(size - (i*CHUNK_SIZE+k*SUB_CHUNK))+" "+ fileName;
            }
            memset(&buff, 0, sizeof(buff));
            strcpy(buff, s.c_str());
            write(peerfd, buff, sizeof(buff));
            memset(&buff, 0, sizeof(buff));
            int resp = read(peerfd, buff, sizeof(buff)); 
            if(resp <= 0){
                cout << "No resp" << endl;
            }     
            outfile.seekp(i*CHUNK_SIZE+k*SUB_CHUNK);
            if(i == numChunks - 1 && k == sub_pieces - 1){
                outfile.write(buff,size - (i*CHUNK_SIZE+k*SUB_CHUNK));
            }else{
                outfile.write(buff,SUB_CHUNK);
            }  
        }
    }
    
    outfile.close();
    char buff[1000];
    memset(&buff, 0, sizeof(buff));
    strcpy(buff, ("d " + fileName).c_str());
    write(peerfd, buff, sizeof(buff));
    memset(&buff, 0, sizeof(buff));
    int resp = recv(peerfd, buff, sizeof(buff),0); 
    dfile(buff,fullPath);
    downloads[fileName].status = "C";
    return NULL;
}

void download_handler(vector<string> &commands,string &filePath){
    vector<peerdetails> peers;
    int numPeers = 0;
    // string filePath = commands[1];
    string fileName = commands[1];
    unsigned int size = stoi(commands[2]);
    int i = 3;
    int n = commands.size();
    while(commands[i] != "file_hash"){
        peerdetails temp;
        temp.ip = commands[i].c_str();
        temp.port = commands[++i].c_str();
        peers.push_back(temp);
        numPeers++;
        i++;
    }
    string hash = commands[++i];
    vector<string> hashChunks;
    int numChunks = 0;
    for(int j = ++i ;j < n; j++){
        numChunks++;
        hashChunks.push_back(commands[j]);
        
    }
    vector<int> peersockets;
    for(int i = 0; i < numPeers;i++){
        char buf[BUF_SIZE];
        struct sockaddr_in server_addr;
        int peerfd = socket(AF_INET, SOCK_STREAM, 0);
        if (peerfd == -1){
            cout << "Socket cannot be created" << endl;
            exit(0);
        }
        memset((char*)&server_addr,0,sizeof(server_addr));
        server_addr.sin_port = htons(atoi(peers[i].port));
        server_addr.sin_family = AF_INET;
        int resp = inet_pton(AF_INET, peers[i].ip, &server_addr.sin_addr);
        if ( resp <= 0)
        {
            cout << "inet_pton problem " << peers[i].ip << endl;
            exit(0);
        }
        if (connect(peerfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
        {
            cout << "peer server connection problem " << peers[i].ip << endl;
            exit(0);
        }
        peersockets.push_back(peerfd);
        commands.clear();

    }
    pthread_t th[numPeers];
    for(int i = 0; i < numPeers;i++){
        userDetails *peer = (userDetails*)malloc(sizeof(userDetails));
        peer->sockfd = peersockets[i];
        peer->path = filePath;
        peer->file_name = fileName;
        peer->hash_of_file = hash;
        peer->size = size;
        peer->hash_of_chunks = hashChunks;
        peer->number_of_chunks = numChunks;
        pthread_create(&th[i],NULL,socket_connection,peer);
        pthread_join(th[i],NULL);
        
    }

}
void *connect_to_tracker(void* tid){
    char buf[BUF_SIZE];
    int trackerfd = *(int*) tid;
    string id;
    while(1){
        string input;
        int i = 0;
        memset(&buf,0,sizeof(buf));
        cout << "\n>: ";
        getline(cin,input);
        if(input == ""){
            cout << "Enter A Command!" << endl;
            continue;
        }
        for(auto c : input){
            buf[i++] = c;
        }
        vector<string> commands;
        parseCommand(buf,commands);
        int n = commands.size();
        string down_path;
        if(commands[0] == "download_file"){
            if(n != 4){
                cout << "correct way: download_file <group_id> <file_name> <destination_path>\n";
                continue;
            }else{
                if(downloads.find(commands[2]) != downloads.end()){
                    cout << "File Already Downloaded" << endl;
                    continue;
                }
                down_path = commands[3];
                id = commands[1];
                string temp = "down " + commands[1]+ " "+commands[2]+" "+this_ip+" "+this_port;
                i = 0;
                memset(&buf,0,sizeof(buf));
                for(auto c : temp){
                    buf[i++] = c;
                }
            }
        }else if(commands[0] == "upload_file"){
            if(n != 3){
                cout << "correct way: upload_file <file_path> <group_id>\n";
                continue;
            }else{
                string path = commands[1];
                string fname = getFileName(path);
                files[fname].size = getFileSize(path);
                files[fname].numChunks = ceil(files[fname].size/ (CHUNK_SIZE));
                files[fname].path = path;
                string temp = "up " + fname+ " "+commands[2] + " "+this_ip+" "+this_port + " "+to_string(files[fname].size);
                temp += " "+genHash(path);
                i = 0;
                memset(&buf,0,sizeof(buf));
                for(auto c : temp){
                    buf[i++] = c;
                }
            }
        }else if(commands[0] == "show_downloads"){
            cout << "Downloading Files:"<<endl;
            for(auto i = downloads.begin();i != downloads.end();i++){
                cout<< "["+i->second.status+"]"+" "+"["+to_string(i->second.id)+"]"+" "+i->first<<endl;
            }
            continue;
        }
            
        write(trackerfd,buf,sizeof(buf));
        memset(&buf,0,sizeof(buf));
        int resp = recv(trackerfd,buf,sizeof(buf),0);
        if(resp <= 0){
            cout << resp <<endl;
            break;
        }
        string temp = buf;
        commands.clear();
        parseCommand(buf,commands);
        
        if(commands[0] == "download"){
            downloads[commands[1]].id = stoi(id);
            downloads[commands[1]].status = "D";
            cout << "Downloading... use show_downloads to check status\n" << endl;
            download_handler(commands,down_path);
        }else{
            cout << temp << endl;
        }
        
        
    }
    return NULL;
}

int main(int argc, char const *argv[])
{
    string x = argv[1];
    size_t colon_pos = x.find(':');
    this_ip = x.substr(0, colon_pos);
    this_port = x.substr(colon_pos+1);
    ifstream file(argv[2]);
    string address, port;
    getline(file,address);
    getline(file,port);
    const char * addr = address.c_str();
    const char * pt = port.c_str();
    int sockfd, connfd;
    int sockfd_s;
    int *peer = (int *)malloc(sizeof(int));
    pthread_t th;
    struct sockaddr_in servaddr_s, client_s;
    struct sockaddr_in server_addr, client;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        cout << "Socket cannot be created" << endl;
        exit(0);
    }
    memset((char*)&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(pt));
    server_addr.sin_addr.s_addr = inet_addr(addr);
    
    if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0)
    {
        cout << "Error in inet_pton for " << addr;
        exit(-1);
    }
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
    {
        printf("connection with the server failed...\n");
        exit(0);
    }
    
    sockfd_s = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_s == -1)
    {
        cout << "Socket cannot be created" << endl;
        exit(0);
    }
    memset(&servaddr_s,0, sizeof(servaddr_s));
    servaddr_s.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr_s.sin_family = AF_INET;
    servaddr_s.sin_port = htons(stoi(this_port));
    if (::bind(sockfd_s, (struct sockaddr *)&servaddr_s, sizeof(servaddr_s)) < 0)
    {
        cout << "socket cannot be binded" << endl;
        exit(0);
    }
    if ((listen(sockfd_s, 5)) != 0)
    {
        printf("Listen failed...\n");
        cout << "Listening failed" << endl;
        exit(0);
    }
    
    *peer = sockfd;
    pthread_create(&th,NULL,connect_to_tracker,peer);
    socklen_t len;
    for(;;)
    {
        int connfd_s = accept(sockfd_s, (struct sockaddr *)&client_s, &len);
        int *clientfd = (int *)malloc(sizeof(int));
        *clientfd = connfd_s;
        if (connfd_s < 0)
        {
            cout << "Server Cannot open socket" << endl;
            exit(0);
        }
        pthread_t p;
        pthread_create(&p,NULL,listen_peer,clientfd);
    }
    pthread_join(th,NULL);

    return 0;
}

