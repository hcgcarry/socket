/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>   //struct stat
#include <unistd.h>
#define bufSize 100000
#define END_MESSAGE	"*=end=*"
#define REQUEST_MESSAGE "*=start=*"
#define errorCheckTher 1000

#define endPreLen 7
int tcpClient(int portno, int ip);
int tcpServer(int portno, int ip, char *filename);
void udpServer(int portno, int ip, char *filename);
void udpClient(int portno, int ip);

char *getFileExtension(char *filename);
void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if(argc < 4){
        fprintf(stderr,"no enough parameter");
    }
    int port = htons(atoi(argv[4]));
    int ip = inet_addr(argv[3]);
    char filename[100];
    char protocol[10];
    strcpy(protocol,argv[1]);
    char sendOrRecv[10];
    strcpy(sendOrRecv, argv[2]);
    
    /*
    int port = htons(5566);
    int ip = inet_addr("127.0.0.1");
    */

    if (!strcmp(protocol, "tcp") && !strcmp(sendOrRecv, "recv")) {
        tcpClient(port, ip);
    } else if (!strcmp(protocol, "tcp") && !strcmp(sendOrRecv, "send")) {
        if(argc < 5){
          fprintf(stderr,"no enough parameter");
        }
        strcpy(filename, argv[5]);
        //strcpy(filename, "test.JPG");
        tcpServer(port, ip, filename);
    } else if (!strcmp(protocol, "udp") && !strcmp(sendOrRecv, "recv")) {
      udpClient(port,ip);
    } else if (!strcmp(protocol, "udp") && !strcmp(sendOrRecv, "send")) {
        if(argc < 5){
          fprintf(stderr,"no enough parameter");
        }
        strcpy(filename, argv[5]);
        //strcpy(filename, "test.JPG");
        udpServer(port, ip, filename);
    }
    return 0;
}
////////////////////////////////////////////////////////tcpServer

int tcpServer(int portno, int ip, char *filename) {
    int sockfd, new_fd, numbytes;
    socklen_t sin_size;
    char buf[bufSize];
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    struct stat filestat;
    FILE *fp;
    int accumulatedSend = 0, percentageCount = 0;
    int tmp = 0;
    time_t timer;
    char timeTmp[26];
    struct tm* tm_info;


    //TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    //Initail, bind to port 2323
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = portno;
    my_addr.sin_addr.s_addr = ip;
    bzero(&(my_addr.sin_zero), 8);

    //binding
    if (bind(sockfd, (struct sockaddr*) &my_addr, sizeof (struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }
    puts("bind success");

    //Start listening ,three way handshake是在這個階段完成，listen queue的每個都會執行handshake
    if (listen(sockfd, 10) == -1) {
        perror("listen");
        exit(1);
    }
    puts("start listening");

    //Get file stat
    if (lstat(filename, &filestat) < 0) {
        exit(1);
    }
    float fivePercentageSize = filestat.st_size * 0.05;
    printf("The file size is %lu\n", filestat.st_size);

    fp = fopen(filename, "rb");
    printf("fp:%p", fp);
    if (fp == NULL) {
        puts("file open error");
        exit(1);
    }

    //Connect
    if ((new_fd = accept(sockfd, (struct sockaddr*) &their_addr, &sin_size)) == -1) {
        perror("accept");
        exit(1);
    }
    puts("accept");
    //send file extension
    strcpy(buf,getFileExtension(filename));
    write(new_fd, buf,strlen(buf) );
    read(new_fd,buf,sizeof(buf));
    if(strcmp(buf,REQUEST_MESSAGE)!=0){
      puts("request message receive failed");
      exit(1);
    }

    //Sending file
    while (!feof(fp)) {
        numbytes = fread(buf, sizeof (char), sizeof (buf), fp);
        numbytes = write(new_fd, buf, numbytes);
        printf("Sending %d bytes\n", numbytes);
        accumulatedSend += numbytes;
        if (accumulatedSend / fivePercentageSize > 0) {
            tmp = (int) (accumulatedSend / fivePercentageSize);
            accumulatedSend = accumulatedSend - tmp*fivePercentageSize;
            time(&timer);
            tm_info = localtime(&timer);
            strftime(timeTmp, 26, "%Y-%m-%d %H:%M:%S", tm_info);
            while (tmp) {
                percentageCount++;
                printf("%d%% %s\n", percentageCount * 5, timeTmp);
                tmp--;
            }
        }
    }
    for (; percentageCount <= 20; percentageCount++) {
        printf("%d%% %s\n", percentageCount * 5, timeTmp);
    }
    puts("");
    puts("send complete");
    close(new_fd);
    close(sockfd);
    return 0;
}

////////////////////////////////////////////////////tcpclient

int tcpClient(int portno, int ip) {
    int sockfd, numbytes;
    char buf[bufSize];
    struct sockaddr_in address;
    FILE *fp;

    //TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    //Initial, connect to port 2323
    address.sin_family = AF_INET;
    address.sin_port = portno;
    address.sin_addr.s_addr = ip;
    bzero(&(address.sin_zero), 8);

    //Connect to server
    if (connect(sockfd, (struct sockaddr*) &address, sizeof (struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }
    puts("connect success");

    //Open file
    //if ( (fp = fopen("receive.txt", "wb")) == NULL){
    read(sockfd, buf, sizeof (buf));
    
    char saveFileName[]="receiveTcp.";
    char *tmp;
    strcat(saveFileName,buf);
    if ((fp = fopen(saveFileName, "wb")) == NULL) {
        perror("fopen");
        exit(1);
    }
    strcpy(buf,REQUEST_MESSAGE);
    //
    write(sockfd,buf,strlen(buf));

    //Receive file from server
    while (1) {
        numbytes = read(sockfd, buf, sizeof (buf));
        printf("read %d bytes, ", numbytes);
        if (numbytes == 0) {
            break;
        }
        numbytes = fwrite(buf, sizeof (char), numbytes, fp);
        //printf("fwrite %d bytes", numbytes);
    }
    puts("");
    puts("file receive");

    fclose(fp);
    close(sockfd);
    return 0;
}

void udpServer(int portno, int ip, char *filename) {
    struct sockaddr_in addr;
    int sockfd, len = 0;
    int addr_len = sizeof (struct sockaddr_in);
    FILE *fp;
    char buf[bufSize];
    struct stat filestat;
    int accumulatedSend = 0, percentageCount = 0;
    int tmp = 0;
    time_t timer;
    char timeTmp[26];
    struct tm* tm_info;


    /* 建立socket，注意必须是SOCK_DGRAM */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    /* 填写sockaddr_in 结构 */
    bzero(&addr, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = portno;
    addr.sin_addr.s_addr = ip; // 接收任意IP发来的数据

    /* 绑定socket */
    if (bind(sockfd, (struct sockaddr *) &addr, sizeof (addr)) < 0) {
        perror("connect");
        exit(1);
    }
    puts("bind socket");
    bzero(buf, sizeof (buf));
    recvfrom(sockfd, buf, sizeof (buf), 0,
            (struct sockaddr *) &addr, &addr_len);
    printf("recvfrom %s\n", inet_ntoa(addr.sin_addr));
    printf("receive:%s\n", buf);
    if (strcmp(buf, REQUEST_MESSAGE)) {
        fprintf(stderr, "wrong request message");
        exit(1);
    }
    char fileExtension[10];
    strcpy(fileExtension,getFileExtension(filename));
    
    sendto(sockfd,fileExtension,strlen(fileExtension) , 0, (struct sockaddr *) &addr, addr_len);
    recvfrom(sockfd, buf, sizeof (buf), 0,
            (struct sockaddr *) &addr, &addr_len);
    if(strcmp(buf,REQUEST_MESSAGE)!=0){
      puts("failed when client send second request message");
      exit(1);
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "wrong when open file");
        exit(1);
    }

    if (lstat(filename, &filestat) < 0) {
        exit(1);
    }

    //get file message
    float fivePercentageSize = filestat.st_size * 0.05;
    printf("The file size is %lu\n", filestat.st_size);
    //Sending file
    int serverGiveErrorCheck = 0, serverGiveTotalNumber = 0;
    while (1) {
        puts("first while##############");
        bzero(buf, sizeof (buf));
        serverGiveErrorCheck = 0;
        serverGiveTotalNumber = 0;
        fseek(fp, 0, SEEK_SET);
        accumulatedSend = 0;
        percentageCount = 0;

        while (!feof(fp)) {
            puts("second while");
            bzero(buf, sizeof (buf));
            len = fread(buf, sizeof (char), sizeof (buf), fp);
            len = sendto(sockfd, buf, len, 0, (struct sockaddr *) &addr, addr_len);
            for (int i = 0; i < len; i++) {
                serverGiveErrorCheck += buf[i];
            }
            serverGiveErrorCheck %= errorCheckTher;
            serverGiveTotalNumber += len;

            printf("Sending %d bytes\n", len);
            accumulatedSend += len;
            if (accumulatedSend / fivePercentageSize > 0) {
                tmp = (int) (accumulatedSend / fivePercentageSize);
                accumulatedSend = accumulatedSend - tmp*fivePercentageSize;
                //取得目前的時間
                time(&timer);
                tm_info = localtime(&timer);
                //時間格式化
                strftime(timeTmp, 26, "%Y-%m-%d %H:%M:%S", tm_info);
                while (tmp) {
                    percentageCount++;
                    printf("%d%% %s\n", percentageCount * 5, timeTmp);
                    tmp--;
                }
            }
        }
        //////send end message with error check and total lenght
        puts("send end message");
        bzero(buf, sizeof (buf));
        strcpy(buf, END_MESSAGE);
        snprintf(buf + strlen(END_MESSAGE), sizeof (buf) - strlen(END_MESSAGE), "%d %d",
            serverGiveTotalNumber, serverGiveErrorCheck);
        sendto(sockfd, buf, len, 0, (struct sockaddr *) &addr, addr_len);
        printf("endmessage have sended:%s\n", buf);
        //等待client返回訊息
        bzero(buf, sizeof (buf));
        recvfrom(sockfd, buf, sizeof (buf), 0,
                (struct sockaddr *) &addr, &addr_len);
        puts("client have response end");
        //如果client接收正確就結束while
        printf("client end response %s\n", buf);
        if (strcmp(buf, "no_problem") == 0) {
            break;
        }
        puts("resent");
    }


    puts("");
    puts("send complete");
    fclose(fp);


}

void udpClient(int portno, int ip) {
    struct sockaddr_in addr;
    int sockfd, len = 0;
    int addr_len = sizeof (struct sockaddr_in);
    FILE *fp;
    char buf[bufSize];



    /* 建立socket，注意必须是SOCK_DGRAM */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    /* 填写sockaddr_in*/
    bzero(&addr, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = portno;
    addr.sin_addr.s_addr = ip;

    sendto(sockfd, REQUEST_MESSAGE, strlen(REQUEST_MESSAGE), 0, (struct sockaddr *) &addr, addr_len);
    //read file extension
    recvfrom(sockfd, buf, sizeof (buf), 0,
            (struct sockaddr *) &addr, &addr_len);
    char filename[]="receiveUdp.";
    strcat(filename,buf);

    sendto(sockfd, REQUEST_MESSAGE, strlen(REQUEST_MESSAGE), 0, (struct sockaddr *) &addr, addr_len);
    if ((fp = fopen(filename, "wb+")) == NULL) {
        perror("fopen");
        exit(1);
    }

    int numBytes = 0;
    puts("file open");

    //Receive file from server
    int totalReceiveBytes = 0;
    int errorCheck = 0;
    int resent = 1;
    char *ptr, *secondParameter, *firstParameter,*fileType;
    int serverGiveErrorCheck = 0, serverGiveTotalNumber = 0;
    while (1) {
        puts("first while###############");
        totalReceiveBytes = 0;
        errorCheck = 0;
        rewind(fp);

        while (1) {
            puts("second while");
            bzero(buf, sizeof (buf));
            numBytes = recvfrom(sockfd, buf, sizeof (buf), 0,
                    (struct sockaddr *) &addr, &addr_len);
            printf("receive %d bytes\n", numBytes);
            //接受並比對server傳過來的end Message
            if (strncmp(buf, END_MESSAGE, endPreLen) == 0) {
                puts("end message conformed");
                ptr = buf;
                ptr = ptr + endPreLen;
                secondParameter = strchr(buf, ' ') + 1;
                /*
                fileType=strchr(buf,' ')+1;
                *(fileType-1)='\0';
                */
                serverGiveErrorCheck = atoi(secondParameter);
                *(secondParameter - 1) = '\0';
                firstParameter = ptr;
                serverGiveTotalNumber = atoi(firstParameter);
                printf("serverT:%d,serverE:%d\n", serverGiveTotalNumber, serverGiveErrorCheck);
                break;
            }
            //將接受到的寫到file裏面
            numBytes = fwrite(buf, sizeof (char), numBytes, fp);
            printf("%d bytes write to file\n", numBytes);
            //totalReceiveBytes 存的事總接受的bytes數如果跟server傳的數
            //一樣就代表沒有漏接data
            totalReceiveBytes += numBytes;
            //errorchek 是將每個char值相加然後%errorCheckTher
            //如果出來的值跟server做同樣操作得到的值一樣就有高機率
            //接收到的值是正確的
            for (int i = 0; i<sizeof (buf); i++) {
                errorCheck += buf[i];
            }
            errorCheck = errorCheck % errorCheckTher;
        }

        if (totalReceiveBytes != serverGiveTotalNumber || errorCheck !=
                serverGiveErrorCheck) {
            puts("resent");
            sendto(sockfd, "resent", 7,
                    0, (struct sockaddr *) &addr, addr_len);
        }//如果接收到的沒有問題的話就跳出回圈結束
        else {
            puts("save end");
            sendto(sockfd, "no_problem", strlen("no_problem"),
                    0, (struct sockaddr *) &addr, addr_len);
            break;
        }
    }
    puts("");
    puts("file receive");

    fclose(fp);



}

char *getFileExtension(char *filename){
  char *extension;
  extension=strchr(filename,'.')+1;
  return extension;
}
