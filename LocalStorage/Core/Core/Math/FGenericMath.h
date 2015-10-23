#pragma once


class FGenericMath
{
public:

    /**
     * Computes the base 2 logarithm for an integer value that is greater than 0.
     * The result is rounded down to the nearest integer.
     *
     * @param Value		The value to compute the log of
     * @return			Log2 of Value. 0 if Value is 0.
     */
    static _AttrAlwaysInline uint32_t floorLog2(uint32_t value)
    {
        /*		
         // reference implementation
         // 1500ms on test data
         uint32 Bit = 32;
         for (; Bit > 0;)
         {
         Bit--;
         if (Value & (1<<Bit))
         {
         break;
         }
         }
         return Bit;
         */
        // same output as reference

        // see http://codinggorilla.domemtech.com/?p=81 or http://en.wikipedia.org/wiki/Binary_logarithm but modified to return 0 for a input value of 0
        // 686ms on test data
        uint32_t pos = 0;
        if (value >= 1<<16) { value >>= 16; pos += 16; }
        if (value >= 1<< 8) { value >>=  8; pos +=  8; }
        if (value >= 1<< 4) { value >>=  4; pos +=  4; }
        if (value >= 1<< 2) { value >>=  2; pos +=  2; }
        if (value >= 1<< 1) {				pos +=  1; }
        return (value == 0) ? 0 : pos;

        // even faster would be method3 but it can introduce more cache misses and it would need to store the table somewhere
        // 304ms in test data
        /*int LogTable256[256];

         void prep()
         {
         LogTable256[0] = LogTable256[1] = 0;
         for (int i = 2; i < 256; i++)
         {
         LogTable256[i] = 1 + LogTable256[i / 2];
         }
         LogTable256[0] = -1; // if you want log(0) to return -1
         }

         int _forceinline method3(uint32 v)
         {
         int r;     // r will be lg(v)
         uint32 tt; // temporaries

         if ((tt = v >> 24) != 0)
         {
         r = (24 + LogTable256[tt]);
         }
         else if ((tt = v >> 16) != 0)
         {
         r = (16 + LogTable256[tt]);
         }
         else if ((tt = v >> 8 ) != 0)
         {
         r = (8 + LogTable256[tt]);
         }
         else
         {
         r = LogTable256[v];
         }
         return r;
         }*/
    }

    /**
     * Counts the number of leading zeros in the bit representation of the value
     *
     * @param Value the value to determine the number of leading zeros for
     *
     * @return the number of zeros before the first "on" bit
     */

    static _AttrAlwaysInline uint32_t countLeadingZeros(uint32_t value)
    {
        if (value == 0) return 32;
        return 31 - floorLog2(value);
    }
    
    /**
     * Returns smallest N such that (1<<N)>=Arg.
     * Note: CeilLogTwo(0)=0 because (1<<0)=1 >= 0.
     */
    static _AttrAlwaysInline uint32_t ceilLogTwo( uint32_t arg )
    {
        int32_t bitmask = ((int32_t)(countLeadingZeros(arg) << 26)) >> 31;
        return (32 - countLeadingZeros(arg - 1)) & (~bitmask);
    }
};