/*
 *  WINGs server.c: example how to create a network server using WMConnection
 *
 *  Copyright (c) 2001 Dan Pascu
 *
 */


#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <WINGs/WINGs.h>


#define _(P)         P
#define MAXCMD_SIZE  512


char *SEConnectionShouldBeRemovedNotification = "SEConnectionShouldBeRemovedNotification";




static void didReceiveInput(ConnectionDelegate *self, WMConnection *cPtr);

static void connectionDidDie(ConnectionDelegate *self, WMConnection *cPtr);

static void connectionDidTimeout(ConnectionDelegate *self, WMConnection *cPtr);


extern char *SEConnectionShouldBeRemovedNotification;

static WMUserDefaults *timeDB = NULL;
static char *ServerAddress = NULL;
static char *ServerPort = NULL;
static WMArray *allowedHostList = NULL;
static WMArray *clientConnections = NULL;
static WMConnection *serverPtr = NULL;



static ConnectionDelegate socketDelegate = {
    NULL,                 /* client data */
    NULL,                 /* didCatchException */
    connectionDidDie,     /* didDie */
    NULL,                 /* didInitialize */
    didReceiveInput,      /* didReceiveInput */
    connectionDidTimeout  /* didTimeout */
};



void
wAbort(Bool foo) /*FOLD00*/
{
    exit(1);
}


static void
printHelp(char *progname) /*FOLD00*/
{
    printf(_("usage: %s [options]\n\n"), progname);
    puts(_(" --help				print this message"));
    puts(_(" --listen [address:]port	only listen on the specified address/port"));
    puts(_(" --allow host1[,host2...]	only allow connections from listed hosts\n"));
    puts(_(" By default server listens on all interfaces and port 34567, unless"
           " something\nelse is specified with the --listen option. If address is"
           " omitted or the keyword\n'Any' is used, it will listen on all interfaces else"
           " only on the specified one.\n\nFor example --listen localhost: will"
           " listen on the default port 34567, but only\non connections comming"
           " in through the loopback interface.\n\n Also by default the server"
           " listens to incoming connections from any host,\nunless a list of"
           " hosts is given with the --allow option, in which case it will\nreject"
           " connections not comming from those hosts.\nThe list of hosts is comma"
           " separated and should NOT contain ANY spaces."));
}


static void
enqueueConnectionForRemoval(WMConnection *cPtr)
{
    WMNotification *notif;

    /*don't release notif here. it will be released by queue after processing */
    notif = WMCreateNotification(SEConnectionShouldBeRemovedNotification,
                                 cPtr, NULL);
    WMEnqueueNotification(WMGetDefaultNotificationQueue(), notif, WMPostASAP);
}


static int
sendMessage(WMConnection *cPtr, char *message)
{
    WMData *aData;
    int res;

    if (WMGetConnectionState(cPtr)!=WCConnected)
        return -1;

    aData = WMCreateDataWithBytes(message, strlen(message));
    res = WMSendConnectionData(cPtr, aData);
    WMReleaseData(aData);

    return res;
}


static Bool
enqueueMessage(WMConnection *cPtr, char *message)
{
    WMData *aData;
    Bool res;

    if (WMGetConnectionState(cPtr)!=WCConnected)
        return False;

    aData = WMCreateDataWithBytes(message, strlen(message));
    res = WMEnqueueConnectionData(cPtr, aData);
    WMReleaseData(aData);

    return res;
}


static unsigned char*
findDelimiter(unsigned char *data, unsigned const char *endPtr)
{
    wassertrv(data < endPtr, NULL);

    while (data<endPtr && *data!='\n' && *data!='\r' && *data!=';' && *data!='\0')
        data++;

    if (data < endPtr)
        return data;

    return NULL;
}


static WMArray*
getAvailableMessages(WMConnection *cPtr)
{
    char *ptr, *crtPos, *buffer;
    const char *bytes, *endPtr;
    WMData *aData, *receivedData, *holdData;
    WMRange range;
    WMArray *messages;
    int length;

    receivedData = WMGetConnectionAvailableData(cPtr);
    if (!receivedData)
        return NULL;
    if ((length=WMGetDataLength(receivedData))==0) {
        WMReleaseData(receivedData);
        return NULL;
    }

    holdData = (WMData*)WMGetConnectionClientData(cPtr);
    if (holdData) {
        WMAppendData(holdData, receivedData);
        WMReleaseData(receivedData);
        WMSetConnectionClientData(cPtr, NULL);
        aData = holdData;
    } else {
        aData = receivedData;
    }

    length = WMGetDataLength(aData);
    bytes = (char*)WMDataBytes(aData);
    endPtr = bytes + length;

    messages = WMCreateArrayWithDestructor(1, wfree);
    crtPos = (char*)bytes;
    while (crtPos<endPtr && (ptr = findDelimiter(crtPos, endPtr))!=NULL) {
        range.position = (crtPos - bytes);
        range.count = (ptr - crtPos);
        if (range.count > MAXCMD_SIZE) {
            /* Hmmm... The message is too long. Possibly that someone is
             * flooding us, or there is a dumb client which do not know
             * who is talking to. */
            sendMessage(cPtr, "Command too long\n\r");
            WMFreeArray(messages);
            WMReleaseData(aData);
            WMCloseConnection(cPtr);
            enqueueConnectionForRemoval(cPtr);
            return NULL;
        }
        buffer = wmalloc(range.count+1);
        WMGetDataBytesWithRange(aData, buffer, range);
        buffer[range.count] = '\0';
        WMAddToArray(messages, buffer);
        crtPos = ptr;
        while (crtPos<endPtr && (*crtPos=='\n' || *crtPos=='\r' ||
                                 *crtPos=='\t' || *crtPos=='\0' ||
                                 *crtPos==';' || *crtPos==' ')) {
            crtPos++;
        }
    }

    if (crtPos<endPtr) {
        range.position = (crtPos - bytes);
        range.count = (endPtr - crtPos);
        if (range.count > MAXCMD_SIZE) {
            /* Flooooooding!!!! */
            sendMessage(cPtr, "Message too long\n\r");
            WMFreeArray(messages);
            WMReleaseData(aData);
            WMCloseConnection(cPtr);
            enqueueConnectionForRemoval(cPtr);
            return NULL;
        }
        holdData = WMGetSubdataWithRange(aData, range);
        WMSetConnectionClientData(cPtr, holdData);
    }
    WMReleaseData(aData);

    if (WMGetArrayItemCount(messages)==0) {
        WMFreeArray(messages);
        messages = NULL;
    }
    return messages;
}



static void
complainAboutBadArgs(WMConnection *cPtr, char *cmdName, char *badArgs) /*FOLD00*/
{
    char *buf = wmalloc(strlen(cmdName) + strlen(badArgs) + 100);

    sprintf(buf, _("Invalid parameters '%s' for command %s. Use HELP for"
                   " a list of commands.\n"), badArgs, cmdName);
    sendMessage(cPtr, buf);
    wfree(buf);
}


static void
sendUpdateMessage(WMConnection *cPtr, char *id, int time) /*FOLD00*/
{
    char *buf = wmalloc(strlen(id) + 100);

    sprintf(buf, "%s has %i minutes left\n", id, time);
    sendMessage(cPtr, buf);
    wfree(buf);
}


static void
showId(WMConnection *cPtr)
{
    sendMessage(cPtr, "Server example based on WMConnection\n");
}


static void
showHelp(WMConnection *cPtr) /*FOLD00*/
{
    char *buf = wmalloc(strlen(WMGetApplicationName()) + 16);

    sprintf(buf, _("%s commands:\n\n"), WMGetApplicationName());

    enqueueMessage(cPtr, _("\n"));
    enqueueMessage(cPtr, buf);
    enqueueMessage(cPtr, _("GET <id>\t- return time left (in minutes) "
                           "for user with id <id>\n"));
    enqueueMessage(cPtr, _("SET <id> <time>\t- set time limit to <time> "
                           "minutes for user with id <id>\n"));
    enqueueMessage(cPtr, _("ADD <id> <time>\t- add <time> minutes "
                           "for user with id <id>\n"));
    enqueueMessage(cPtr, _("SUB <id> <time>\t- subtract <time> minutes "
                           "for user with id <id>\n"));
    enqueueMessage(cPtr, _("REMOVE <id>\t- remove time limitations for "
                           "user with id <id>\n"));
    enqueueMessage(cPtr, _("LIST\t\t- list all users and their "
                           "corresponding time limit\n"));
    enqueueMessage(cPtr, _("ID\t\t- returns the Time Manager "
                           "identification string\n"));
    enqueueMessage(cPtr, _("EXIT\t\t- exits session\n"));
    enqueueMessage(cPtr, _("QUIT\t\t- exits session\n"));
    enqueueMessage(cPtr, _("HELP\t\t- show this message\n\n"));
    /* Just flush the queue we made before */
    WMFlushConnection(cPtr);
    wfree(buf);
}


static void
listUsers(WMConnection *cPtr)
{
    proplist_t userList;
    char *id;
    int i, time;

    userList = WMGetUDAllKeys(timeDB);

    for (i=0; i<PLGetNumberOfElements(userList); i++) {
        id = PLGetString(PLGetArrayElement(userList, i));
        time = WMGetUDIntegerForKey(timeDB, id);
        sendUpdateMessage(cPtr, id, time);
    }

    PLRelease(userList);
}


static void
setTimeForUser(WMConnection *cPtr, char *cmdArgs) /*FOLD00*/
{
    char *id;
    int i, time;

    id = wmalloc(strlen(cmdArgs));
    if (sscanf(cmdArgs, "%s %d", id, &time)!=2) {
        complainAboutBadArgs(cPtr, "SET", cmdArgs);
        wfree(id);
        return;
    }
    if (time<0)
        time = 0;

    WMSetUDIntegerForKey(timeDB, time, id);

    for (i=0; i<WMGetArrayItemCount(clientConnections); i++) {
        cPtr = WMGetFromArray(clientConnections, i);
        sendUpdateMessage(cPtr, id, time);
    }
    wfree(id);
}


static void
addTimeToUser(WMConnection *cPtr, char *cmdArgs) /*FOLD00*/
{
    char *id;
    int i, time, newTime;

    id = wmalloc(strlen(cmdArgs));
    if (sscanf(cmdArgs, "%s %d", id, &time)!=2) {
        complainAboutBadArgs(cPtr, "ADD", cmdArgs);
        wfree(id);
        return;
    }

    newTime = WMGetUDIntegerForKey(timeDB, id) + time;
    if (newTime<0)
        newTime = 0;

    WMSetUDIntegerForKey(timeDB, newTime, id);

    for (i=0; i<WMGetArrayItemCount(clientConnections); i++) {
        cPtr = WMGetFromArray(clientConnections, i);
        sendUpdateMessage(cPtr, id, newTime);
    }
    wfree(id);
}


static void
subTimeFromUser(WMConnection *cPtr, char *cmdArgs) /*FOLD00*/
{
    char *id;
    int i, time, newTime;

    id = wmalloc(strlen(cmdArgs));
    if (sscanf(cmdArgs, "%s %d", id, &time)!=2) {
        complainAboutBadArgs(cPtr, "SUB", cmdArgs);
        wfree(id);
        return;
    }

    newTime = WMGetUDIntegerForKey(timeDB, id) - time;
    if (newTime<0)
        newTime = 0;

    WMSetUDIntegerForKey(timeDB, newTime, id);

    for (i=0; i<WMGetArrayItemCount(clientConnections); i++) {
        cPtr = WMGetFromArray(clientConnections, i);
        sendUpdateMessage(cPtr, id, newTime);
    }
    wfree(id);
}


static void
removeTimeForUser(WMConnection *cPtr, char *cmdArgs) /*FOLD00*/
{
    char *ptr;
    int i;

    if (cmdArgs[0]=='\0') {
        sendMessage(cPtr, _("Missing parameter for command REMOVE."
                            " Use HELP for a list of commands.\n"));
        return;
    }

    ptr = cmdArgs;
    while (*ptr && *ptr!=' ' && *ptr!='\t')
        ptr++;
    *ptr = '\0';

    WMRemoveUDObjectForKey(timeDB, cmdArgs);

    for (i=0; i<WMGetArrayItemCount(clientConnections); i++) {
        cPtr = WMGetFromArray(clientConnections, i);
        sendUpdateMessage(cPtr, cmdArgs, -1);
    }
}


static void
getTimeForUser(WMConnection *cPtr, char *cmdArgs) /*FOLD00*/
{
    char *ptr;
    int time;

    if (cmdArgs[0]=='\0') {
        sendMessage(cPtr, _("Missing parameter for command GET."
                            " Use HELP for a list of commands.\n"));
        return;
    }

    ptr = cmdArgs;
    while (*ptr && *ptr!=' ' && *ptr!='\t')
        ptr++;
    *ptr = '\0';

    if (WMGetUDObjectForKey(timeDB, cmdArgs)!=NULL)
        time = WMGetUDIntegerForKey(timeDB, cmdArgs);
    else
        time = -1;

    sendUpdateMessage(cPtr, cmdArgs, time);
}


static void
handleConnection(WMConnection *cPtr)
{
    char *command, *ptr, *cmdArgs, *buffer;
    WMArray *commands;
    int i;

    commands = getAvailableMessages(cPtr);
    if (!commands)
        return;

    for (i=0; i<WMGetArrayItemCount(commands); i++) {
        command = WMGetFromArray(commands, i);
        while (*command && (*command==' ' || *command=='\t'))
            command++;
        ptr = command;
        while(*ptr && *ptr!=' ' && *ptr!='\t')
            ptr++;
        if (*ptr) {
            *ptr = '\0';
            ptr++;
        }
        while (*ptr && (*ptr==' ' || *ptr=='\t'))
            ptr++;

        cmdArgs = ptr;

        fprintf(stderr, "Command: '%s', args: '%s'\n", command, cmdArgs);

        if (strcasecmp(command, "quit")==0 || strcasecmp(command, "exit")==0) {
            sendMessage(cPtr, "Bye\n");
            WMCloseConnection(cPtr);
            enqueueConnectionForRemoval(cPtr);
            WMFreeArray(commands);
            return;
        } else if (strcasecmp(command, "id")==0) {
            showId(cPtr);
        } else if (strcasecmp(command, "help")==0) {
            showHelp(cPtr);
        } else if (strcasecmp(command, "list")==0) {
            listUsers(cPtr);
        } else if (strcasecmp(command, "set")==0) {
            setTimeForUser(cPtr, cmdArgs);
        } else if (strcasecmp(command, "add")==0) {
            addTimeToUser(cPtr, cmdArgs);
        } else if (strcasecmp(command, "sub")==0) {
            subTimeFromUser(cPtr, cmdArgs);
        } else if (strcasecmp(command, "remove")==0) {
            removeTimeForUser(cPtr, cmdArgs);
        } else if (strcasecmp(command, "get")==0) {
            getTimeForUser(cPtr, cmdArgs);
        } else {
            buffer = wmalloc(strlen(command) + 100);
            sprintf(buffer, _("Unknown command '%s'. Try HELP for"
                              " a list of commands.\n"), command);
            sendMessage(cPtr, buffer);
            wfree(buffer);
        }
    }

    WMFreeArray(commands);
}


static Bool
isAllowedToConnect(WMConnection *cPtr)
{
    WMHost *hPtr;
    int i;

    if (allowedHostList == NULL)
        return True; /* No list. Allow all by default */

    hPtr = WMGetHostWithAddress(WMGetConnectionAddress(cPtr));
    for (i=0; i<WMGetArrayItemCount(allowedHostList); i++) {
        if (WMIsHostEqualToHost(hPtr, WMGetFromArray(allowedHostList, i))) {
            WMReleaseHost(hPtr);
            return True;
        }
    }

    WMReleaseHost(hPtr);

    return False;
}


static void
didReceiveInput(ConnectionDelegate *self, WMConnection *cPtr) /*FOLD00*/
{
    if (cPtr == serverPtr) {
        WMConnection *newPtr = WMAcceptConnection(cPtr);

        if (newPtr) {
            if (isAllowedToConnect(newPtr)) {
                WMSetConnectionDelegate(newPtr, &socketDelegate);
                WMSetConnectionSendTimeout(newPtr, 120);
                WMAddToArray(clientConnections, newPtr);
            } else {
                sendMessage(newPtr, "Sorry, you are not allowed to connect.\n");
                WMDestroyConnection(newPtr);
            }
        }
    } else {
        /* Data arriving on an already-connected socket */
        handleConnection(cPtr);
    }
}


static void
connectionDidTimeout(ConnectionDelegate *self, WMConnection *cPtr) /*FOLD00*/
{
    WMHost *hPtr;

    if (cPtr == serverPtr) {
        wfatal(_("The server listening socket did timeout. Exiting."));
        exit(1);
    }

    hPtr = WMGetHostWithAddress(WMGetConnectionAddress(cPtr));
    wwarning(_("Connection with %s did timeout. Closing connection."),
             WMGetHostName(hPtr));
    WMReleaseHost(hPtr);

    enqueueConnectionForRemoval(cPtr);
}


static void
connectionDidDie(ConnectionDelegate *self, WMConnection *cPtr)
{
    if (cPtr == serverPtr) {
        /* trouble. server listening port itself died!!! */
        wfatal(_("The server listening socket died. Exiting."));
        exit(1);
    }

    enqueueConnectionForRemoval(cPtr);
}


static void
removeConnection(void *observer, WMNotification *notification)
{
    WMConnection *cPtr = (WMConnection*)WMGetNotificationObject(notification);
    WMData *data;

    WMRemoveFromArray(clientConnections, cPtr);
    if ((data = (WMData*)WMGetConnectionClientData(cPtr))!=NULL)
        WMReleaseData(data);
    WMDestroyConnection(cPtr);
}


#if 0
static Bool
isDifferent(char *str1, char *str2) /*FOLD00*/
{
    if ((!str1 && !str2) || (str1 && str2 && strcmp(str1, str2)==0))
        return False;

    return True;
}
#endif


int
main(int argc, char **argv) /*FOLD00*/
{
    int i;

    wsetabort(wAbort);

    WMInitializeApplication("server", &argc, argv);

    if (argc>1) {
        for (i=1; i<argc; i++) {
            if (strcmp(argv[i], "--help")==0) {
                printHelp(argv[0]);
                exit(0);
            } else if (strcmp(argv[i], "--listen")==0) {
                char *p;

                if ((p = strchr(argv[++i], ':')) != NULL) {
                    *p = 0;
                    ServerAddress = wstrdup(argv[i]);
                    ServerPort = wstrdup(p+1);
                    *p = ':';
                    if (ServerAddress[0] == 0) {
                        wfree(ServerAddress);
                        ServerAddress = NULL;
                    }
                    if (ServerPort[0] == 0) {
                        wfree(ServerPort);
                        ServerPort = "34567";
                    }
                } else if (argv[i][0]!=0) {
                    ServerPort = argv[i];
                }
            } else if (strcmp(argv[i], "--allow")==0) {
                char *p, *ptr;
                int done;
                WMHost *hPtr;

                ptr = argv[++i];
                done = 0;
                while (!done) {
                    if ((p = strchr(ptr, ',')) != NULL) {
                        *p = 0;
                    }
                    if (*ptr != 0) {
                        hPtr = WMGetHostWithName(ptr);
                        if (hPtr) {
                            if (!allowedHostList)
                                allowedHostList = WMCreateArray(4);
                            WMAddToArray(allowedHostList, hPtr);
                        } else {
                            wwarning(_("Unknown host '%s'. Ignored."), ptr);
                        }
                    }

                    if (p!=NULL) {
                        *p = ',';
                        ptr = p+1;
                    } else {
                        done = 1;
                    }
                }
            } else {
                printf(_("%s: invalid argument '%s'\n"), argv[0], argv[i]);
                printf(_("Try '%s --help' for more information\n"), argv[0]);
                exit(1);
            }
        }
    }

    timeDB = WMGetDefaultsFromPath("./UserTime.plist");

    clientConnections = WMCreateArray(4);

    /* A NULL ServerAddress means to listen on any address the host has.
     * Else if ServerAddress points to a specific address (like "localhost",
     * "host.domain.com" or "192.168.1.1"), then it will only listen on that
     * interface and ignore incoming connections on the others. */
    if (ServerAddress && strcasecmp(ServerAddress, "Any")==0)
        ServerAddress = NULL;
    if (ServerPort==NULL)
        ServerPort = "34567";

    printf("Server will listen on '%s:%s'\n", ServerAddress?ServerAddress:"Any",
           ServerPort);
    printf("This server will allow connections from:");
    if (allowedHostList) {
        int i;
        char *hName;

        for (i=0; i<WMGetArrayItemCount(allowedHostList); i++) {
            hName = WMGetHostName(WMGetFromArray(allowedHostList, i));
            printf("%s'%s'", i==0?" ":", ", hName);
        }
        printf(".\n");
    } else {
        printf(" any host.\n");
    }

    serverPtr = WMCreateConnectionAsServerAtAddress(ServerAddress, ServerPort,
                                                    NULL);

    if (!serverPtr) {
        wfatal("could not create server on `%s:%s`. Exiting.",
               ServerAddress ? ServerAddress : "localhost", ServerPort);
        exit(1);
    }

    WMSetConnectionDelegate(serverPtr, &socketDelegate);

    WMAddNotificationObserver(removeConnection, NULL,
                              SEConnectionShouldBeRemovedNotification, NULL);

    while (1) {
        /* The ASAP notification queue is called at the end of WHandleEvents()
         * There's where died connections we get while running through
         * WHandleEvents() get removed. */
        WHandleEvents();
    }

    return 0;
}


