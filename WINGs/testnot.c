
/*
 * test notifications
 */


#include "WUtil.h"
#include <stdio.h>
#include <stdlib.h>


char *notificationA = "notificationA";
char *notificationB = "notificationB";
char *notificationC = "notificationC";

void
observer1(void *data, WMNotification *notification)
{
    printf("ObserverAction 1 got %s with object=%s, clientdata=%s\n",
	   WMGetNotificationName(notification),
	   (char*)WMGetNotificationObject(notification),
	   (char*)WMGetNotificationClientData(notification));
}

void
observer3(void *data, WMNotification *notification)
{
    printf("ObserverAction 3 got %s with object=%s, clientdata=%s\n",
	   WMGetNotificationName(notification),
	   (char*)WMGetNotificationObject(notification),
	   (char*)WMGetNotificationClientData(notification));
}

void
observer2(void *data, WMNotification *notification)
{
    printf("ObserverAction 2 got %s with object=%s, clientdata=%s\n",
	   WMGetNotificationName(notification),
	   (char*)WMGetNotificationObject(notification),
	   (char*)WMGetNotificationClientData(notification));
}





int
main(int argc, char **argv)
{
    int i;
    char *obser1 = "obser1";
    char *obser2 = "obser2";
    char *obser3 = "obser3";
    char *obj1 = "obj1";
    char *obj2 = "obj2";
    char *obj3 = "obj3";
    char *cdata1 = "client data1";
    char *cdata2 = "client data2";
    char *cdata3 = "client data3";

    WMInitializeApplication("test", &argc, argv);

    WMAddNotificationObserver(observer1, obser1, notificationA, obj1);

    WMAddNotificationObserver(observer1, obser1, notificationA, obj2);

    WMAddNotificationObserver(observer1, obser1, notificationA, obj3);

    puts("post1");
    WMPostNotificationName(notificationA, obj3, cdata1);
    puts("post2");
    WMPostNotificationName(notificationA, obj2, cdata2);
    puts("post3");
    WMPostNotificationName(notificationA, obj1, cdata3);

    puts("REMOVE1");
    WMRemoveNotificationObserverWithName(obser1, notificationA, obj2);

    puts("REMOVE2");
    WMRemoveNotificationObserverWithName(obser1, notificationA, obj2);
    
    puts("post1");
    WMPostNotificationName(notificationA, obj3, cdata1);
    puts("post2");
    WMPostNotificationName(notificationA, obj2, cdata2);
    puts("post3");
    WMPostNotificationName(notificationA, obj1, cdata3);

    return 0;
}

