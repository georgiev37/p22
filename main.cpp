
#include <list>
#include "orderUpdate.h"
#include "levelVolume.h"

struct id_hash //hashing for IDPair
{
	template <class T1, class T2>
	std::size_t operator() (const std::pair<T1, T2> &pair) const
	{
		return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
	}
};

class FeedHandler
{
    public:
        typedef std::pair<int64_t, int64_t> IDPair; //only clientID and orderID together are unique
        typedef std::unordered_map<int64_t, LevelVolumeByTime*> PriceLevelMap; //used for cacheing previous volume per price level
        typedef std::unordered_map<IDPair, OrderUpdate*, id_hash> OrderMap; //Both buys and sells
        typedef std::mutex order_lock;
        typedef std::lock_guard<std::mutex> mutex_lock;
    
    private:
       int64_t m_next_sequence;
       OrderMap m_orders;
       PriceLevelMap m_levels;
       order_lock m_orderLock;
       
       std::list<OrderUpdate*> m_out_of_sequence; // we cant garauntee any order of messages. Will likely need to delete and insert in middle of container

    public:
        FeedHandler():
            m_next_sequence(1)
        {
            std::cout<< "Feed Handler Created! " << std::endl;
        }

        inline void onSocketUpdate(OrderUpdate *update)
        {
            mutex_lock lock(m_orderLock);
            if (m_next_sequence != update->sequence)
            {
                std::cout<< "Out of order Seq. expected " << m_next_sequence << " Got " << update->sequence 
                    <<std::endl;
                m_out_of_sequence.push_back(update);
                return;
            }
            else
            {
                onExchangeUpdate(update);
                m_next_sequence++;
                std::cout<< "Next expected seq:" << m_next_sequence << std::endl;
                return;
            }
        }
    
        inline void onExchangeUpdate( OrderUpdate *update ) 
        {
            switch (update->type)
            {
                case UpdateType::New:
                {
                    onExchangeNew(update);
                    break;
                }
                case UpdateType::Change:
                {
                    onExchangeModify(update);
                    break;
                }
                case UpdateType::Cancel:
                {
                    onExchangeCancel(update);
                    break;
                }                   
                default:
                    break;
            }

        };

        inline void applyQueuedMessages()//called in main test function. in practice called after snapshot recovery/etc
        {
            mutex_lock lock(m_orderLock);
            auto it = m_out_of_sequence.begin();

            while( it!= m_out_of_sequence.end())
            {
                OrderUpdate *pOrd = *it;

                if (m_next_sequence == pOrd->sequence)
                {
                    std::cout<<"recovered sequence " << pOrd->sequence << std::endl;
                    onExchangeUpdate(pOrd);
                    m_out_of_sequence.erase(it++);
                    m_next_sequence++;
                }
                it++;
            }
        }

        inline void onExchangeNew(OrderUpdate *update)
        {      
            m_orders[IDPair(update->clientId, update->orderId)] = update;
            //debug
            auto side = update->side == Side::Buy? "BUY" : "SELL";
            std::cout<<"Inserted clientID " << update->clientId << " orderId " 
                << update->orderId << " Side " << side << " Quantity: " << update->quantity << " Price: " << update->price
                << ". Total Orders: " << m_orders.size() <<  std::endl;
            //end debug
            auto level = m_levels.find(update->price);
            if (level == m_levels.end())
            {
                std::cout<<"New Level. Adding " << update->quantity << " to price " << update->price << std::endl;
                LevelVolumeByTime *lev = new LevelVolumeByTime();
                lev->addVolume(update->entryTimestamp, update->quantity);
                m_levels[update->price] = lev;
            }
            else
            {
                std::cout<<"Existing Level. Adding " << update->quantity << " to price " << update->price << std::endl;
                auto pLev = level->second;
                pLev->addVolume(update->entryTimestamp, update->quantity);
                m_levels[update->price] = pLev;
                
            }
        };

        inline void onExchangeModify(OrderUpdate *update)
        {
            //modify existing order
            //clean up Level *
        };

        inline void onExchangeCancel(OrderUpdate *update)
        {
            //cancel existing order
            //clean up Level *
        };

        inline OrderUpdate* getOrderUpdate(const int64_t clientID, const int64_t orderID)
        {
            mutex_lock lock(m_orderLock);
            auto it = m_orders.find(IDPair(clientID, orderID));
            if (it!= m_orders.end())
            {
                return it->second;
            }
            return nullptr;
        };

        inline LevelVolumeByTime* getLevel(const int64_t price)
        {
            auto it = m_levels.find(price);
            if (it != m_levels.end())
            {
                return it->second;
            }
            return nullptr;
        }

        inline int64_t getPreviousVolume(const int64_t clientID, const int64_t orderID)
        {
            auto order = getOrderUpdate(clientID, orderID);
            if (order != nullptr)
            {
                mutex_lock lock(m_orderLock);       
                auto lev = getLevel(order->price);
                if (lev!= nullptr)
                {
                    return lev->getVolumeAtTimestamp(order->entryTimestamp);
                }
                else{
                    //handle error
                    return -1;
                }
            }
            else{
                //handle error
                return -1;
            }
        }

        inline void printOrders() const
        {
            std::cout<< "\t Next expected sequence: " << m_next_sequence << std::endl;
            std::cout<< "\t orders" << std::endl;

            for (const auto &entry: m_orders)
            {
                std::cout << "\t {" << entry.first.first<< "," << entry.first.second << "}: "
                        << entry.second->quantity << "@" << entry.second->price << std::endl;
            }   
        }
};

int main(int argc, char *argv[])
{
    std::cout << " Beginning Feed Handler " << std::endl;
    auto f = FeedHandler();
    
    //add some fake exchange orders, use the socket callback
    OrderUpdate order = OrderUpdate(1, UpdateType::New, 411, 1, 1, 1, Side::Buy, 101, 40);
    f.onSocketUpdate(&order);

    //out of sequence order
    OrderUpdate bad_order = OrderUpdate(3, UpdateType::New, 411, 3, 3, 1, Side::Buy, 103, 40);
    f.onSocketUpdate(&bad_order);

    OrderUpdate good_order = OrderUpdate(2, UpdateType::New, 411, 2, 2, 1, Side::Buy, 102, 40);
    f.onSocketUpdate(&good_order);
    f.printOrders();

    f.applyQueuedMessages(); //realistically called after recovery

    f.printOrders();

    //add another order at price level 101
    OrderUpdate good_order2 = OrderUpdate(4, UpdateType::New, 411, 4, 4, 1, Side::Buy, 101, 60);
    f.onSocketUpdate(&good_order2);
    auto previous_qty = f.getPreviousVolume(good_order2.clientId, good_order2.orderId);
    assert(previous_qty == order.quantity); // the first order had 40 lots

    f.printOrders();

    auto good = f.getOrderUpdate(411, 1);
    if (good != nullptr)
    {
        std::cout << "Found: {" << good->clientId << "," << good->orderId << "}: "
                        << good->quantity << "@" << good->price << std::endl;
    }
    //find order by clientID and orderID
    assert(f.getOrderUpdate(order.clientId, order.orderId) == &order); 
    assert(f.getOrderUpdate(412,1 ) == nullptr);
    
    std::cout <<"Tests complete" << std::endl;
    return 0;
}