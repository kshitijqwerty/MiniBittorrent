# MiniBittorrent

This project is a small scale version of torrent where you can share and download your file accordingly . 
You can become a seeder when you download a file and upload that file to all others who wants to download.

## 1. First of all mTorrent file details:
	sha1 (hash of hash of the file)	
	tracker URL1 
	tracker URL2
	filename
	filesize (in Bytes)
SHA1 of the file you wish to share to the tracker so that others can download it , is made dividing the file into logical pieces of 512KB.
Whenever you share the file to the tracker this program will create the <filename>.mtorrent that would be mentioned on the command line.

## 2. Torrent Tracker
A tracker which is a special type of server helps in communication between two clients.
whenever you share a file that is sent to a tracker and tracker stores the necessary details of the file and of you socket to help the other clients download that file .

Whenever you seek to download a file tracker has a seederlist functionality which whill send you the information of the seeders i.e. socket address. All socket addresses will be shared to the client whenever you seek to download a file .

You can also remove the details of the file that you have already sent to the tracker. And tracker will hence not provide your details to any other client who wants to download the same file.

The tracker will be able to handle multiple request at a time and it would not be shut down .

## 3. Torrent Clients

That is you , can share a file and become a seeder henceforth or you can download a file and then become a seeder. 

You can download a file from multiple clients. Multithreading has been used in this project .

Whenever you need to download a file you will get the socket adresses from the tracker and requests for the download will be generated to all those 
socket addresses. 

Those seeders will give you a bitmap according to which the algorithm written will decide from whom to take which pieces of file(512KB).

That algorithm is dynamic and will help you download your file faster as it will take optimal number of pieces of file from all the seeders.

After a get or share again a get or a share will be performed the system would not be blocked.

Propper mapping of filepaths and their corresponding .mtorrents are maintained.

## SYNTAX :

Start client using :
``./client <CLIENT_IP>:<UPLOAD_PORT> <TRACKER_IP_1>:<TRACKER_PORT_1> <TRACKER_IP_2>:<TRACKER_PORT_2>``

SHARING a local file:

Command  : ``share <local file path> <filename>.mtorrent``

DOWNLOADING a file :

Command: ``get <path to .mtorrent file> <destination path>``

SHOW DOWNLOADS:

Command: ``show downloads``

REMOVE :
Command: ``remove <filename.mtorrent>``







