/*    Copyright 2009 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "Precompiled.h"

namespace _bson {

    using std::dec;
    using std::endl;
    using std::hex;
    using std::numeric_limits;
    using std::set;
    using std::string;
    using std::stringstream;

    FBSONElement eooElement;

    /** transform a BSON array into a vector of BSONElements.
        we match array # positions with their vector position, and ignore
        any fields with non-numeric field names.
        */
    FArray<FBSONElement> FBSONElement::Array() const {
        chk(_bson::Array);
        FArray<FBSONElement> v;
        FBSONObjIterator i(Obj());
        while( i.more() ) {
            FBSONElement e = i.next();
            const char *f = e.fieldName();

            unsigned u;
            Status status = _bson::parseNumberFromString( f, &u );
            if ( status.isOK() ) {
                verify( u < 1000000 );
                if( u >= v.size() )
                    v.resize(u+1);
                v[u] = e;
            }
            else {
                // ignore?
            }
        }
        return FMove(v);
    }

    /* grab names of all the fields in this object */
    int FBSONObj::getFieldNames(set<string>& fields) const
    {
        int n = 0;
        FBSONObjIterator i(*this);
        while ( i.moreWithEOO() )
        {
            FBSONElement e = i.next();
            if ( e.eoo() )
                break;
            fields.insert(e.fieldName());
            n++;
        }
        return n;
    }

    bool FBSONObjBuilder::appendAsNumber( const StringData& fieldName , const string& data )
    {
        if ( data.size() == 0 || data == "-" || data == ".")
            return false;

        unsigned int pos=0;
        if ( data[0] == '-' )
            pos++;

        bool hasDec = false;

        for ( ; pos<data.size(); pos++ )
        {
            if ( isdigit(data[pos]) )
                continue;

            if ( data[pos] == '.' )
            {
                if ( hasDec )
                    return false;
                hasDec = true;
                continue;
            }

            return false;
        }

        if ( hasDec )
        {
            double d = atof( data.c_str() );
            append( fieldName , d );
            return true;
        }

        if ( data.size() < 8 )
        {
            append( fieldName , atoi( data.c_str() ) );
            return true;
        }

        try
        {
            long long num = boost::lexical_cast<long long>( data );
            append( fieldName , num );
            return true;
        }
        catch(boost::bad_lexical_cast &)
        {
            return false;
        }
    }

    /* take a BSONType and return the name of that type as a char* */
    const char* typeName (BSONType type)
    {
        switch (type)
        {
            case MinKey: return "MinKey";
            case EOO: return "EOO";
            case NumberDouble: return "NumberDouble";
            case String: return "String";
            case Object: return "Object";
            case Array: return "Array";
            case BinData: return "BinaryData";
            case Undefined: return "Undefined";
            case jstOID: return "OID";
            case Bool: return "Bool";
            case Date: return "Date";
            case jstNULL: return "NULL";
            case RegEx: return "RegEx";
            case DBRef: return "DBRef";
            case Code: return "Code";
            case Symbol: return "Symbol";
            case CodeWScope: return "CodeWScope";
            case NumberInt: return "NumberInt32";
            case Timestamp: return "Timestamp";
            case NumberLong: return "NumberLong64";
            // JSTypeMax doesn't make sense to turn into a string; overlaps with highest-valued type
            case MaxKey: return "MaxKey";
            default: return "Invalid";
        }
    }
    
}