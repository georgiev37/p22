#ifndef LEVELVOLUME_H
#define LEVELVOLUME_H
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <iostream>


class LevelVolumeByTime
{    
    private:
        typedef std::unordered_map <int64_t, int64_t> TimeStampVol;
        typedef std::lock_guard<std::mutex> mutex_lock;
        int64_t m_lastQty;
        int64_t m_lastEntryTS;
        TimeStampVol m_timeVol;
        std::mutex m_volLock;

    public:
        LevelVolumeByTime():
            m_lastEntryTS(0), m_lastQty(0){}

        inline void addVolume(const int64_t entryTimestamp, const int64_t quantity)
        {
                //This is a bit ambiguous per requirements. Buy and sell volume is being treated equally here per the requirements 
                // Canceled orders are never backed out as live orders only is not specified, just orders placed prior to timestamp
                //this logic could be very different if getting the volume is for determining queue priority
                if (entryTimestamp > m_lastEntryTS)
                {
                    m_lastEntryTS = entryTimestamp;
                    m_timeVol[entryTimestamp]= m_lastQty;
                    m_lastQty += quantity;
                    std::cout << "Added " << quantity << " at TS " << entryTimestamp << " levelSize: " << m_timeVol.size() <<std::endl;
                    return;
                }
                else{
                    //handle error
                    return;
                }
        }

        inline int64_t getVolumeAtTimestamp(const int64_t timestamp)
        {
            std::cout<< "Requesting volume before timestamp:  " << timestamp << std::endl;
            auto it = m_timeVol.find(timestamp);
            if (it != m_timeVol.end())
            {
                return it->second;
            }
            else{
                //handle error
                return -1;
            }
        }
        
};
#endif