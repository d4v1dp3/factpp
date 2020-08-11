#ifndef FACT_ByteOrder
#define FACT_ByteOrder

#include <string.h>
#include <arpa/inet.h>

#include <vector>
#include <algorithm>
#include <typeinfo>
#include <stdexcept>

template<typename S>
void reset(S &s)
{
    memset(&s, 0, sizeof(S));
}

template<typename S>
void init(S &s)
{
    if (sizeof(S)%2!=0)
        throw std::logic_error("size of "+std::string(typeid(S).name())+" not a multiple of 2.");

    reset(s);
}

template<typename S>
void hton(S &s)
{
    std::transform(reinterpret_cast<uint16_t*>(&s),
                   reinterpret_cast<uint16_t*>(&s)+sizeof(S)/2,
                   reinterpret_cast<uint16_t*>(&s),
                   htons);
}

template<typename S>
void ntoh(S &s)
{
    std::transform(reinterpret_cast<uint16_t*>(&s),
                   reinterpret_cast<uint16_t*>(&s)+sizeof(S)/2,
                   reinterpret_cast<uint16_t*>(&s),
                   ntohs);
}

template<typename S>
S NtoH(const S &s)
{
    S ret(s);
    ntoh(ret);
        return ret;
}

template<typename S>
S HtoN(const S &s)
{
    S ret(s);
    hton(ret);
    return ret;
}

template<typename S>
void ntohcpy(const std::vector<uint16_t> &vec, S &s)
{
    if (sizeof(S)!=vec.size()*2)
        throw std::logic_error("ntohcpy: size of vector mismatch "+std::string(typeid(S).name()));

    std::transform(vec.begin(), vec.end(),
                   reinterpret_cast<uint16_t*>(&s), ntohs);
}

template<typename S>
std::vector<uint16_t> htoncpy(const S &s)
{
    if (sizeof(S)%2)
        throw std::logic_error("htoncpy: size of "+std::string(typeid(S).name())+" not a multiple of 2");

    std::vector<uint16_t> v(sizeof(S)/2);

    std::transform(reinterpret_cast<const uint16_t*>(&s),
                   reinterpret_cast<const uint16_t*>(&s)+sizeof(S)/2,
                   v.begin(), htons);

    return v;
}

template<typename T, typename S>
void bitcpy(T *target, size_t ntarget, const S *source, size_t nsource, size_t ss=0, size_t ts=0)
{
    const size_t targetsize = ts==0 ? sizeof(T)*8 : std::min(ts, sizeof(T)*8);
    const size_t sourcesize = ss==0 ? sizeof(S)*8 : std::min(ss, sizeof(S)*8);

    const S *const ends = source + nsource;
    const T *const endt = target + ntarget;

    const S *s = source;
    T *t = target;

    memset(t, 0, sizeof(T)*ntarget);

    size_t targetpos = 0;
    size_t sourcepos = 0;

    while (s<ends && t<endt)
    {
        // Start filling with "source size" - "position" bits
        *t |= (*s>>sourcepos)<<targetpos;

        // Calculate how many bits were siuccessfully copied
        const int ncopy = std::min(sourcesize-sourcepos, targetsize-targetpos);

        targetpos += ncopy;
        sourcepos += ncopy;

        if (sourcepos>=sourcesize)
        {
            sourcepos %= sourcesize;
            s++;
        }

        if (targetpos>=targetsize)
        {
            targetpos %= targetsize;
            t++;
        }

    }
}

template<typename T>
void Reverse(T *t)
{
    std::reverse((uint16_t*)t, ((uint16_t*)t)+sizeof(T)/2);
}

#endif
