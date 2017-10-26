//
// Created by matt on 26/10/17.
//

#ifndef ADS_CDEC_MAIN_H
#define ADS_CDEC_MAIN_H

#include <QtCore/QtGlobal>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QVector>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QMetaEnum>
#include <QtCore/QBitArray>
#include <QtCore/QtMath>
#include <QtCore/QString>

#define lat_scaller                     0.000171661376953125
#define long_scaller                    0.000171661376953125
#define alt_scaller                     4.0
#define time_scaller                    0.125
#define truetrack_scaller               0.087890625
#define trueheading_scaller             0.087890625
#define groundspeed_scaller             0.5
#define machspeed_scaller               0.0005
#define verticalrate_scaller            16
#define windspeed_scaller               0.5
#define truewinddirection_scaller       0.703125
#define temperature_scaller             0.25
#define distance_scaller                0.125

struct DownlinkHeader
{
    bool valid;
    QString flightid;
    QString OriginatorString;
    int MessageNumber;
    char BlockSequenceCharacter;
    void clear()
    {
        valid=false;
        flightid.clear();
        OriginatorString.clear();
        MessageNumber=-1;
        BlockSequenceCharacter=-1;
    }
};

struct ArincMessage
{
    bool valid;//if crc is true
    bool downlink;//if is downlink
    QString info;//human readable message
    QString IMI;//ADS AT1 etc
    QString tailno;//REG
    void clear()
    {
        valid=false;
        downlink=false;
        info.clear();
        IMI.clear();
        tailno.clear();
    }
};

typedef enum ADSDownlinkMessages {
    Acknowledgement=3,
    Negative_Acknowledgement=4,
    Noncompliance_Notification=5,
    Cancel_Emergency_Mode=6,
    Basic_Report=7,
    Emergency_Basic_Report=9,
    Lateral_Deviation_Change_Event=10,
    Flight_Identification_Group=12,
    Predicted_Route_Group=13,
    Earth_Reference_Group=14,
    Air_Reference_Group=15,
    Meteorological_Group=16,
    Airframe_Identification_Group=17,
    Vertical_Rate_Change_Event=18,
    Altitude_Range_Event=19,
    Waypoint_Change_Event=20,
    Intermediate_Projected_Intent_Group=22,
    Fixed_Projected_Intent_Group=23
} ADSDownlinkMessages;


struct DownlinkBasicReportGroup
{
    //Message type
    ADSDownlinkMessages messagetype;
    //AES
    quint32 AESID;
    //Downlink header
    DownlinkHeader downlinkheader;
    //Lat
    double latitude;
    //Long
    double longitude;
    //Alt
    double altitude;
    //Time stamp (seconds past the hour)
    double time_stamp;
    //FOM
    int FOM;
    //Message valid
    bool valid;
};

struct DownlinkEarthReferenceGroup
{
    //AES
    quint32 AESID;
    //Downlink header
    DownlinkHeader downlinkheader;
    //True track is valid
    bool truetrack_isvalid;
    //True track
    double truetrack;
    //Ground speed
    double groundspeed;
    //Vertical rate
    double verticalrate;
    //Message valid
    bool valid;
};

//all messages must contain a basic report group so should always be valid other groups may or maynot be valid
struct DownlinkGroups
{
    DownlinkBasicReportGroup adownlinkbasicreportgroup;
    DownlinkEarthReferenceGroup adownlinkearthreferencegroup;
    void clear()
    {
        adownlinkbasicreportgroup.valid=false;
        adownlinkearthreferencegroup.valid=false;
    }
    bool isValid()
    {
        if(adownlinkbasicreportgroup.valid)return true;
        return false;
    }
};



#endif //ADS_CDEC_MAIN_H
