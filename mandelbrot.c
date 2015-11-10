//Jordan Jacobson and Ari Tchetchenian
//Tutorial: Tue12, Oliver Tan
//24.04.2015

//Serves a .BMP of the mandelbrot set at various
//x, y coordinates and zoom

#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include "pixelColor.h"           //Contains function definitions 
                                  //for the color genorator
#include "mandelbrot.h"           //Contains function definition 
                                  //for the escape steps



//SERVER RELATED DEFINES:
#define REQUEST_BUFFER_SIZE 10000       //Size of request from browser

#define DEFAULT_PORT 7191               //Port number

#define NUMBER_OF_PAGES_TO_SERVE 10000  //Number of pages to deliver
                                        //before shutdown


//MANDELBROT SET RELATED DEFINES:
#define ITERATION_MAX 256               //Number of iterations 
                                        //before a point is
                                        //declared out of set

#define SIZE 512                        //Number of pixels in 
                                        //rows and columns


#define ESCAPE_THRESHOLD 2              //Limit threshold of function
                                        //to determine whether point
                                        //is in the set or not


//BMP HEADER RELATED DEFINES
#define BMP_INDICATION 0x4d42              //magic nums indicate a BMP
#define BYTES_PER_PIXEL 3
#define BITS_PER_PIXEL (BYTES_PER_PIXEL*8)
#define COMPRESSION 0                      //zero for no compression
#define BMP_HEADER_SIZE 54                 //size of BMPs main header
#define DIB_HEADER_SIZE 40                 //size of BMPs DIB header
#define PIXELS_PER_METRE 2835              //typically left at 2835
#define NUMBER_OF_COLORS 0                 //0 for max possible colors
#define NUMBER_OF_REQUIRED_COLORS 0        //0 for all colors required

typedef unsigned char bits8;            //can store 8 bits of data         
typedef unsigned short bits16;          //can store 16 bits of data
typedef unsigned int bits32;            //can store 32 bits of data

typedef struct _complex{                //Current point in the plane
    double x;
    double y;
} complex;



//--------Function Definitions--------//
int startServer(void);
void servePage(int numberServedCounter, int serverSocket);
int waitForConnection (int serverSocket);
int makeServerSocket (int portno);
void serveBMP (int socket,char request[]);
void writePixel (int socket, int iterations, bits8 byte);
static void serveHTML (int socket);
void writeBMPHeader(int socket);
void writeHTTPHeader(int socket);



//--------Mainline--------//
int main (int argc, char *argv[]) {
    int numberServedCounter;
    int serverSocket;

    //check that these store 1, 2 & 4 bytes as required
    assert (sizeof(bits8) == 1);
    assert (sizeof(bits16) == 2);
    assert (sizeof(bits32) == 4);

    serverSocket = startServer();

    numberServedCounter = 0;

    while (numberServedCounter < NUMBER_OF_PAGES_TO_SERVE){
        servePage(numberServedCounter, serverSocket);
        numberServedCounter++;
    }

    // close the server connection after we are done
    close (serverSocket);

    return EXIT_SUCCESS; 
}



//---Finds connection with server---//
int startServer(void){

    int serverSocket = makeServerSocket (DEFAULT_PORT);   
    return serverSocket;
    
}



//--------Delivers Web Page----------//
void servePage(int numberServedCounter, int serverSocket){

    char request[REQUEST_BUFFER_SIZE];

    int connectionSocket = waitForConnection (serverSocket);
    //Wait for a request to be sent from a web browser, 
    //open a new connection for this conversation

    //Read the first line of the request sent by the browser  
    int bytesRead;
    bytesRead = read (connectionSocket, request, (sizeof request)-1);
    assert (bytesRead >= 0); 

    //If the fifth charictor is 't' then the request
    //is for a single bmp image. Otherwise, deliver
    //the HTML viewer    
    if (request[5] == 't'){
        serveBMP(connectionSocket, request);
    } else {
        serveHTML (connectionSocket);
    }

    // close the connection after sending the page
    close(connectionSocket);
}


//--------Delivers BMP Image---------//
void serveBMP (int socket, char request[]) {
    int row = -SIZE/2;    //Defines where each row should start
    int col = -SIZE/2;    //Defines where first column should start

    int iterations = 0;   //Count the amount of iterations needed
                          //to escape the set
    
    complex current;      //The current coodinates of the point
                          //being tested

    int colCounter = 0;   //Current collum
    int rowCounter = 0;   //Current row
    
    int zoom = 0;         //The zoom level of the image
    double xMiddle = 0;   //The x (real) component of the 
                          //center of the image
    double yMiddle = 0;   //The y (imaginary) component of the 
                          //center of the image

    bits8 byte = 0;       //Intensity of color from each sub-pixel
    
    writeHTTPHeader(socket);
    writeBMPHeader(socket);

    //get coordinates from URL
    sscanf (request, "GET /tile_x%lf_y%lf_z%d.bmp",
            &xMiddle,&yMiddle,&zoom);

    //print each pixel out
    while (rowCounter < SIZE){
        current.y = yMiddle + row*exp2(-1*zoom) + exp2(-1*zoom)/2;
        while (colCounter < SIZE){
            current.x = xMiddle + col*exp2(-1*zoom) + exp2(-1*zoom)/2;

            iterations = escapeSteps(current.x,current.y);

            writePixel (socket,iterations, byte);

            col++;
            colCounter++;
        }
        col = -SIZE/2;
        colCounter = 0;

        row++;
        rowCounter++;
    }

}


//--Calculates Escape Steps for a Point---//
int escapeSteps(double x, double y){
    int counter = 0;

    complex current;
    current.x = x;
    current.y = y;

    complex temp;
    complex original;

    original.x = current.x;
    original.y = current.y;

    while (((current.x*current.x + current.y*current.y) < 
            ESCAPE_THRESHOLD*ESCAPE_THRESHOLD) && 
            (counter <ITERATION_MAX)){
        temp.x = (current.x)*(current.x) - (current.y)*(current.y) 
        + original.x;
        current.y = 2*(current.x)*(current.y) + original.y;
        current.x = temp.x;
        counter++;
    }

    return counter;
}


//------Writes BMP Header-----//
void writeBMPHeader(int socket){

//Bitmap main header
    bits16 bmpIndication = BMP_INDICATION;
    write(socket, &bmpIndication, sizeof(bmpIndication));

    bits32 fileSize = BMP_HEADER_SIZE + (SIZE*SIZE*BYTES_PER_PIXEL);
    write(socket, &fileSize, sizeof(fileSize));

    bits32 reservedBits = 0;
    write(socket, &reservedBits, sizeof(reservedBits));

    bits32 numPixelsInHeader = BMP_HEADER_SIZE;
    write(socket, &numPixelsInHeader, sizeof(numPixelsInHeader));

//Bitmap DIB header (extra data required for printing .BMP to screen)
    bits32 dibHeaderSize = DIB_HEADER_SIZE;
    write(socket, &dibHeaderSize, sizeof(dibHeaderSize));

    bits32 imageWidth = SIZE;
    write(socket, &imageWidth, sizeof(imageWidth));

    bits32 imageHeight = SIZE;
    write(socket, &imageHeight, sizeof(imageHeight));

    bits16 numberOfPlanes = 1;
    write(socket, &numberOfPlanes, sizeof(numberOfPlanes));

    bits16 bitsPerPixel = BITS_PER_PIXEL;
    write(socket, &bitsPerPixel, sizeof(bitsPerPixel));

    bits32 compression = COMPRESSION;
    write(socket, &compression, sizeof(compression));

    bits32 imageSize = SIZE * SIZE * BYTES_PER_PIXEL;
    write(socket, &imageSize, sizeof(imageSize));

    bits32 xPixelsPerMetre = PIXELS_PER_METRE;
    write(socket, &xPixelsPerMetre, sizeof(xPixelsPerMetre));

    bits32 yPixelsPerMetre = PIXELS_PER_METRE;
    write(socket, &yPixelsPerMetre, sizeof(yPixelsPerMetre));

    bits32 numberOfColors = NUMBER_OF_COLORS;
    write(socket, &numberOfColors, sizeof(numberOfColors));

    bits32 numRequiredColors = NUMBER_OF_REQUIRED_COLORS;
    write(socket, &numRequiredColors, sizeof(numRequiredColors));

}


//----Writes a Single Pixel----//
void writePixel (int socket, int iterations, bits8 byte){

    if (iterations == ITERATION_MAX){

        byte = stepsToRed(iterations);
        write (socket, &byte, sizeof(byte));

        byte = stepsToBlue(iterations);
        write (socket, &byte, sizeof(byte));

        byte = stepsToGreen(iterations);
        write (socket, &byte, sizeof(byte));

    } else {

        byte = stepsToRed(iterations);
        write (socket, &byte, sizeof(byte));

        byte = stepsToBlue(iterations);
        write (socket, &byte, sizeof(byte));

        byte = stepsToGreen(iterations);
        write (socket, &byte, sizeof(byte));

    }
}


//-----Delivers HTML Viewer-----//
static void serveHTML (int socket) {
   char* message;
 
   // first send the http response header
   message =
      "HTTP/1.0 200 Found\n"
      "Content-Type: text/html\n"
      "\n";

   write (socket, message, strlen (message));
 
   message =
   "<!DOCTYPE html>\n"
   "<script src=\"http://almondbread.cse.unsw.edu.au/tiles.js\">"
   "</script>"
    "\n";      
   write (socket, message, strlen (message));
}



//------Starts Server Listening to Port-------//
int makeServerSocket (int portNumber) { 

    // create socket
    int serverSocket = socket (AF_INET, SOCK_STREAM, 0);
    assert (serverSocket >= 0);   
    // error opening socket

    // bind socket to listening port
    struct sockaddr_in serverAddress;
    bzero ((char *) &serverAddress, sizeof (serverAddress));

    serverAddress.sin_family      = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port        = htons (portNumber);

    // let the server start immediately after a previous shutdown
    int optionValue = 1;
    setsockopt (serverSocket,SOL_SOCKET,SO_REUSEADDR,
                &optionValue,sizeof(int));
    int bindSuccess = bind (serverSocket, (struct sockaddr *) 
                            &serverAddress,sizeof (serverAddress));

    assert (bindSuccess >= 0);
    // if this assert fails wait a short while to let the operating 
    // system clear the port before trying again

    return serverSocket;
}


//-----Writes HTTP Header-----//
void writeHTTPHeader(int socket){
    char* message;

    message = "HTTP/1.0 200 OK\r\n"
    "Content-Type: image/bmp\r\n"
    "\r\n";

    write (socket, message, strlen (message));
}


//-----Returns Socket Number-----//
int waitForConnection (int serverSocket) {
    // listen for a connection
    const int serverMaxBacklog = 10;
    listen (serverSocket, serverMaxBacklog);

    // accept the connection
    struct sockaddr_in clientAddress;
    socklen_t clientLen = sizeof (clientAddress);

    int connectionSocket = 
    accept (serverSocket, (struct sockaddr *) &clientAddress, 
    &clientLen);

    assert (connectionSocket >= 0);
    // error on accept

    return (connectionSocket);
}
