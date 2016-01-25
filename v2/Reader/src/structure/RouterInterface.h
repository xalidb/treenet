/*
 * RouterInterface.h
 *
 *  Created on: Oct 28, 2015
 *      Author: grailet
 *
 * A simple class to represent a single interface of an inferred router. Originally, a Router 
 * object would consist in a list of InetAddress objects, but in order to incorporate more details 
 * (e.g. on alias resolution), this class has been added.
 *
 * N.B.: originating from TreeNET ("Full"), this class is almost identical to what can be found 
 * in the former software, except for one method.
 */

#ifndef ROUTERINTERFACE_H_
#define ROUTERINTERFACE_H_

#include "../common/inet/InetAddress.h"

class RouterInterface
{
public:

    // Possible methods being used when this IP is associated to a router
    enum AliasMethod
    {
        NOT_ALIASED, 
        FIRST_IP, 
        ALLY, 
        IPID_VELOCITY, 
        REVERSE_DNS
    };

    RouterInterface(InetAddress ip, unsigned short aliasMethod);
    ~RouterInterface();
    
    InetAddress ip;
    unsigned short aliasMethod;
    
    // Comparison method
    inline static bool smaller(RouterInterface *r1, RouterInterface *r2) { return r1->ip < r2->ip; }
};

#endif /* ROUTERINTERFACE_H_ */