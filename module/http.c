#define _XOPEN_SOURCE 700
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "modules.h"
#include "http.h"
#include "../include/vm.h"

/* Get a file from a webserver
 *
 * based off of the wget.c source code */
static Value httpGetNative(int argCount, Value* args) 
{
 char buffer[BUFSIZ];
 enum CONSTEXPR { MAX_REQUEST_LEN = 1024};
 char request[MAX_REQUEST_LEN];
 char request_template[] = "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
 struct protoent *protoent;
 char *hostname = "mt-lang.org";
 in_addr_t in_addr;
 int request_len;
 int socket_file_descriptor;
 ssize_t nbytes_total, nbytes_last;
 struct hostent *hostent;
 struct sockaddr_in sockaddr_in;
 unsigned short server_port = 80;

 if (argCount == 0) 
 {
  printf("Warning: Hostname not set, defaulting to 'mt-lang.org'.\n");
 }

 // if hostname is not provided we default to 'mt-lang.org'
 if (argCount > 0) {
   if (!IS_STRING(args[0])) 
   {
     runtimeError("Error, expected string as first argument to 'http.Get'.");
     return NIL_VAL;
   } 
   hostname = AS_CSTRING(args[0]);
 }

  // if port is not set default to 80
  if (argCount > 1) 
  {
    if (!IS_NUMBER(args[1])) 
    {
      runtimeError("Error, expected number as seconf argument to 'http.Get'.");
      return NIL_VAL;
    }
    // set the port
    server_port = AS_NUMBER(args[1]); 
  }

  /* join all parts of the request together */
  request_len = snprintf(request, MAX_REQUEST_LEN, request_template, hostname);
  if (request_len >= MAX_REQUEST_LEN) 
  {
    runtimeError("Error, request length given to 'http.Get' too large: %d\n", request_len);
    return NIL_VAL;
  }

  /* Build the socket. */
  protoent = getprotobyname("tcp");
  if (protoent == NULL) 
  {
    runtimeError("Could not build socket for 'http.Get'.");
    return NIL_VAL;
  }

  socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
  if (socket_file_descriptor == -1) 
  {
    runtimeError("Could not build socket for 'http.Get'.");
    return NIL_VAL;
  }

  /* Build the address */
  hostent = gethostbyname(hostname);
  if (hostent == NULL) 
  {
    runtimeError("Error, could not resove hostname '%s' for 'http.Get'", hostname);
    return NIL_VAL;
  }

  in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
  if (in_addr == (in_addr_t)-1) {
    runtimeError("Rrror, inet_addr(\"%s\") in 'http.Get'", *(hostent->h_addr_list));
    return NIL_VAL;
  }

  sockaddr_in.sin_addr.s_addr = in_addr;
  sockaddr_in.sin_family = AF_INET;
  sockaddr_in.sin_port = htons(server_port);

  /* Actually connect, finally. */
  if (connect(socket_file_descriptor, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) 
  {
    runtimeError("Could not connect to http server for 'http.Get'"); 
    return NIL_VAL;
  }

  /* Send HTTP request. */
  nbytes_total = 0;
  while (nbytes_total < request_len) {
    nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
    if (nbytes_last == -1) {
      runtimeError("Error, could not make http request in 'http.Get'");
      return NIL_VAL;
    }
    nbytes_total += nbytes_last;
  }

  /* Read the response. */
  while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0) {
    // write(STDOUT_FILENO, buffer, nbytes_total);
    continue;
  }
  if (nbytes_total == -1) {
    runtimeError("Could not read http respone in 'http.Get'");
    return NIL_VAL;
  }

  close(socket_file_descriptor);
  return OBJ_VAL(copyString(buffer, strlen(buffer)));
}




/* Finally we create the module */
void createHttpModule() 
{
  // name of the overall module
  ObjString* name = copyString("http", 4);
  push(OBJ_VAL(name));

  // we use the name to create the object
  ObjNativeClass *klass = newNativeClass(name);
  push(OBJ_VAL(name));

  defineModuleMethod(klass, "Get", httpGetNative);

  tableSet(&vm.globals, name, OBJ_VAL(klass));
  pop();
  pop();
}

