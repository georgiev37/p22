#ifndef ORDERUPDATE_H
#define ORDERUPDATE_H
#include <cstdint>
#include "side.h"
#include "updateType.h"

struct OrderUpdate {
	int64_t sequence;
	UpdateType type;		
    int64_t clientId;	
    int64_t orderId;		 
    int64_t entryTimestamp;
    int64_t instrumentId;
    Side 	  side;
    int64_t price;		
	int64_t quantity; 
 
    OrderUpdate(int64_t sequence, UpdateType type, int64_t clientId, int64_t orderId, int64_t entryTimestamp, 
                int64_t instrumentId, Side side, int64_t price, int64_t quantity):
                sequence(sequence),
                type(type),
                clientId(clientId),
                orderId(orderId),
                entryTimestamp(entryTimestamp),
                instrumentId(instrumentId),
                side(side),
                price(price),
                quantity(quantity)
                {
                }
};
#endif