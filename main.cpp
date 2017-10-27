#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <sstream>
#include "main.h"
#include <fstream>

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

using namespace std;

struct MessageContents{
    bool has_position = false;
    bool has_waypoint = false;
    string flight;
    void reset()
    {
        has_position = false;
        has_waypoint = false;
        flight.clear();
    }
};

qint32 extractqint32(QByteArray &ba,int lsbyteoffset,int bitoffset,int numberofbits,bool issigned);
void decode_ADS(QString ADSString, ArincMessage* arincmessage, MessageContents* messageContents);

int main() {
    //string in = "142170D82C0588C9DF5C1D0D2184C02D2D48CA0048218A282D9288CA008D04";
    //QString in = "ADS.D-ABYR142170D82C0588C9DF5C1D0D2184C02D2D48CA0048218A282D9288CA008D04";
    //QString in2 = "ADS.9V-SRM07EE31C287234908A33C1D0DEB3AB2940B890887ECEA45EA978145BD400E36F0F4C004100DAADEA8C3F4";
    //QString in4 = "/BOMASAI.ADS.N182UA0720699D271587D037DD9D0D21EB751DE2C7D003C8224D0D1B6E47D0000E76610A3FFC0F74B1A8FFFC1037A39E78CF70";
    //QString in4 = "/BOMASAI.ADS.VT-ANJ0720F4F02D96C9C3EE129F0E5C98EC3FFC0F5CD9A13FFC100EB29E6E9BB5";

    ArincMessage arincmessage;

    MessageContents msgcon;

//    decode_ADS(in4, &arincmessage4, &msgcon);
//
//    cout << arincmessage4.info.toStdString() << endl;
//    cout << "Pos? " << msgcon.has_position << endl;
//    cout << "Way? " << msgcon.has_waypoint << endl;

    ifstream infile("/home/matt/CLionProjects/ADS-CDec/adsc_msg2.txt");

    cout << infile.fail() << endl;

    int total = 0, only_pos = 0, only_way = 0, both = 0, none = 0;

    for(string input_line; getline(infile, input_line);)
    {
        QString msg = QString::fromStdString(input_line.substr(1,input_line.length()-2));
        msgcon.reset();
        arincmessage.clear();

        decode_ADS(msg, &arincmessage, &msgcon);
        cout << arincmessage.info.toStdString() << endl;
        total++;
        if(msgcon.has_position && !msgcon.has_waypoint) only_pos++;
        else if(!msgcon.has_position && msgcon.has_waypoint) only_way++;
        else if(msgcon.has_position && msgcon.has_waypoint) both++;
        else none;

        //TODO Interface with SQLite so we can analyse the per-aircraft metrics.
    }

    infile.close();

    cout << "Total ADS-C Messages: " << total << endl;
    cout << "ADS-C with only position " << only_pos << endl;
    cout << "ADS-C with only waypoint " << only_way << endl;
    cout << "ADS-C with both position and waypoint " << both << endl;
    cout << "ADS-C with neither " << none << endl;

    return 0;
}

//returns true if something for the user to read. need to check what is valid on return
void decode_ADS(QString ADSString, ArincMessage* arincmessage, MessageContents* messageContents)//QString &msg)
{
    QStringList sections=ADSString.split('/');
//if(sections.size()!=2)return true;

    QString MFI_ctraddr=sections[1].section('.',0,0);
    QString IMI_tailno_appmessage_CRC=sections[1].section('.',1);
    QString MFI;
    QString ctraddr=MFI_ctraddr;
//    if(MFI_ctraddr.contains(' '))//incase MFI is present (H1 label rather than the MFI label) happens when ACARS peripheral created message rather than by an ACARS [C]MU
//    {
//        MFI=MFI_ctraddr.section(' ',0,0);// eg B6 if there. its not there in this example as the ACARS header would have come with a B6 in the ACARS label
//        ctraddr=MFI_ctraddr.section(' ',1);// eg AKLCDYA
//    } else MFI=acarsitem.LABEL;// eg B6

//QString IMI_tailno_appmessage_CRC=ADSString;
    QString appmessage=IMI_tailno_appmessage_CRC.mid(3+7,IMI_tailno_appmessage_CRC.size()-3-7-4);//eg 07EEE19454DAC7D010D21D0DEEEC44556208024029F0588C71D7884D000E13B90F00000F12C1A280001029305F10

            //qDebug()<<"ADS packet";

    QByteArray appmessage_bytes=QByteArray::fromHex(appmessage.toLatin1());
    bool fail = false;

    QString middlespacer="  ";

    DownlinkGroups downlinkgroups;

    for(int i=0;(i<appmessage_bytes.size())&&(!fail);)
    {
        switch(appmessage_bytes.at(i))
        {
            case Acknowledgement:
            {
                if((i+2)>appmessage_bytes.size()){fail=true;break;}//sz check
                arincmessage->info+="Acknowledgement";
                arincmessage->info+=((QString)" ADS Contract Request Number = %1\n").arg((uchar)appmessage_bytes.at(i-1+2));
                i+=2;//goto next message
                break;
            }
            case Negative_Acknowledgement:
            {
                if((i+4)>appmessage_bytes.size()){fail=true;break;}//sz check
                arincmessage->info+="Negative_Acknowledgement";
                arincmessage->info+=((QString)" ADS Contract Request Number = %1 Reason = %2\n").arg((uchar)appmessage_bytes.at(i-1+2)).arg((((QString)"%1").arg((uchar)appmessage_bytes.at(i-1+3),2, 16, QChar('0'))).toUpper());
                i+=4;//goto next message
                break;
            }
            case Predicted_Route_Group:
            {
                if((i+18)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Predicted_Route_Group";

                //for next waypoint. cant be bothered to do +1 waypoint just the next will do. feel free to add the +1
                //Lat
                double latitude=((double)extractqint32(appmessage_bytes,i-1+4,3,21,true))*lat_scaller;
                //Long
                double longitude=((double)extractqint32(appmessage_bytes,i-1+7,6,21,true))*long_scaller;
                //Alt
                double altitude=((double)extractqint32(appmessage_bytes,i-1+9,6,16,true))*alt_scaller;
                //ETA (is this time till arrival or time at arrival??)
                double eta=((double)extractqint32(appmessage_bytes,i-1+10,0,14,false));

                arincmessage->info+=middlespacer+((QString)"Next waypoint Lat = %1 Long = %2 Alt = %3 feet. ETA = %4\n").arg(latitude).arg(longitude).arg(altitude).arg(QDateTime::fromTime_t(eta).toUTC().toString("hh:mm:ss"));
                messageContents->has_waypoint = true;
                i+=18;//goto next message
                break;
            }
            case Meteorological_Group:
            {
                if((i+5)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Meteorological_Group";

                //Wind speed
                double windspeed=((double)extractqint32(appmessage_bytes,i-1+3,7,9,false))*windspeed_scaller;
                //True wind direction
                bool truewinddirection_isvalid=!((((uchar)appmessage_bytes.at(i-1+3))>>6)&0x01);
                double truewinddirection=((double)extractqint32(appmessage_bytes,i-1+4,5,9,true))*truewinddirection_scaller;
                if(truewinddirection<0)truewinddirection+=360.0;
                //Temperature (arcinc 745 seems to make a mistake with their egampe on this one)
                double temperature=((double)extractqint32(appmessage_bytes,i-1+5,1,12,true))*temperature_scaller;

                if(truewinddirection_isvalid)arincmessage->info+=middlespacer+((QString)"Wind speed = %1 knots. True wind direction = %2 deg. Temperature = %3 deg C.\n").arg(qRound(windspeed)).arg(qRound(truewinddirection)).arg(temperature);
                else arincmessage->info+=middlespacer+((QString)"Wind speed = %1 knots. Temperature = %2 deg C.\n").arg(qRound(windspeed)).arg(temperature);


                i+=5;//goto next message
                break;
            }
            case Air_Reference_Group:
            {
                if((i+6)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Air_Reference_Group";

                //True heading
                bool trueheading_isvalid=!((((uchar)appmessage_bytes.at(i-1+2))>>7)&0x01);
                double trueheading=((double)extractqint32(appmessage_bytes,i-1+3,3,12,true))*trueheading_scaller;
                if(trueheading<0)trueheading+=360.0;
                //Mach speed
                double machspeed=((double)extractqint32(appmessage_bytes,i-1+5,6,13,false))*machspeed_scaller;
                //Vertical rate
                double verticalrate=((double)extractqint32(appmessage_bytes,i-1+6,2,12,true))*verticalrate_scaller;

                if(trueheading_isvalid)arincmessage->info+=middlespacer+((QString)"True heading = %1 deg. Mach speed = %2 Vertical rate = %3 fpm.\n").arg(qRound(trueheading)).arg(qRound(machspeed*100.0)/100.0).arg(verticalrate);
                else arincmessage->info+=middlespacer+((QString)"Mach speed = %1 Vertical rate = %2 fpm.\n").arg(qRound(machspeed*100.0)/100.0).arg(verticalrate);


                i+=6;//goto next message
                break;
            }
            case Earth_Reference_Group:
            {
                if((i+6)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Earth_Reference_Group";


                //True track
                bool truetrack_isvalid=!((((uchar)appmessage_bytes.at(i-1+2))>>7)&0x01);
                double truetrack=((double)extractqint32(appmessage_bytes,i-1+3,3,12,true))*truetrack_scaller;
                if(truetrack<0)truetrack+=360.0;
                //Ground speed
                double groundspeed=((double)extractqint32(appmessage_bytes,i-1+5,6,13,false))*groundspeed_scaller;
                //Vertical rate
                double verticalrate=((double)extractqint32(appmessage_bytes,i-1+6,2,12,true))*verticalrate_scaller;

                if(truetrack_isvalid)arincmessage->info+=middlespacer+((QString)"True Track = %1 deg. Ground speed = %2 knots. Vertical rate = %3 fpm.\n").arg(qRound(truetrack)).arg(qRound(groundspeed)).arg(verticalrate);
                else arincmessage->info+=middlespacer+((QString)"Ground speed = %1 knots. Vertical rate = %2 fpm.\n").arg(qRound(groundspeed)).arg(verticalrate);

                //populate group
                //downlinkgroups.adownlinkearthreferencegroup.AESID=acarsitem.isuitem.AESID;
                //downlinkgroups.adownlinkearthreferencegroup.downlinkheader=downlinkheader;
                downlinkgroups.adownlinkearthreferencegroup.truetrack=truetrack;
                downlinkgroups.adownlinkearthreferencegroup.truetrack_isvalid=truetrack_isvalid;
                downlinkgroups.adownlinkearthreferencegroup.groundspeed=groundspeed;
                downlinkgroups.adownlinkearthreferencegroup.verticalrate=verticalrate;
                downlinkgroups.adownlinkearthreferencegroup.valid=true;

                i+=6;//goto next message
                break;
            }
            case Flight_Identification_Group:
            {
                if((i+7)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Flight_Identification_Group";

                QByteArray ba(8,' ');
                ba[0]=extractqint32(appmessage_bytes,i-1+2,2,6,false);
                ba[1]=extractqint32(appmessage_bytes,i-1+3,4,6,false);
                ba[2]=extractqint32(appmessage_bytes,i-1+4,6,6,false);
                ba[3]=extractqint32(appmessage_bytes,i-1+4,0,6,false);
                ba[4]=extractqint32(appmessage_bytes,i-1+5,2,6,false);
                ba[5]=extractqint32(appmessage_bytes,i-1+6,4,6,false);
                ba[6]=extractqint32(appmessage_bytes,i-1+7,6,6,false);
                ba[7]=extractqint32(appmessage_bytes,i-1+7,0,6,false);
                for(int k=0;k<8;k++)if((uchar)ba[k]<=26)ba[k]=ba[k]|0x40;

                arincmessage->info+=middlespacer+"Flight ID "+ba.trimmed()+"\n";
                messageContents->flight = ba.trimmed().toStdString();
                i+=7;//goto next message
                break;
            }
            case Basic_Report:
            case Emergency_Basic_Report:
            case Lateral_Deviation_Change_Event:
            case Vertical_Rate_Change_Event:
            case Altitude_Range_Event:
            case Waypoint_Change_Event:
            {
                if((i+11)>appmessage_bytes.size()){fail=true;break;}//sz check
                switch(appmessage_bytes.at(i))
                {
                    case Basic_Report:
                        arincmessage->info+="Basic_Report:\n";
                        break;
                    case Emergency_Basic_Report:
                        arincmessage->info+="Emergency_Basic_Report:\n";
                        break;
                    case Lateral_Deviation_Change_Event:
                        arincmessage->info+="Lateral_Deviation_Change_Event:\n";
                        break;
                    case Vertical_Rate_Change_Event:
                        arincmessage->info+="Vertical_Rate_Change_Event:\n";
                        break;
                    case Altitude_Range_Event:
                        arincmessage->info+="Altitude_Range_Event:\n";
                        break;
                    case Waypoint_Change_Event:
                        arincmessage->info+="Waypoint_Change_Event:\n";
                        break;
                }

                //Lat
                double latitude=((double)extractqint32(appmessage_bytes,i-1+4,3,21,true))*lat_scaller;
                //Long
                double longitude=((double)extractqint32(appmessage_bytes,i-1+7,6,21,true))*long_scaller;
                //Alt
                double altitude=((double)extractqint32(appmessage_bytes,i-1+9,6,16,true))*alt_scaller;
                //Time stamp
                double time_stamp=((double)extractqint32(appmessage_bytes,i-1+11,7,15,false))*time_scaller;
                QTime ts=QDateTime::fromTime_t(qRound(time_stamp)).toUTC().time();
                QString ts_str=(ts.toString("mm")+"m "+ts.toString("ss")+"s");
                //FOM
                int FOM=((uchar)appmessage_bytes.at(i-1+11))&0x1F;

                arincmessage->info+=middlespacer+((QString)"Lat = %1 Long = %2 Alt = %3 feet. Time past the hour = %4 FOM = %5\n").arg(latitude).arg(longitude).arg(altitude).arg(ts_str).arg((((QString)"%1").arg(FOM,2, 16, QChar('0'))).toUpper());
                messageContents->has_position = true;
                //populate group
                //downlinkgroups.adownlinkbasicreportgroup.AESID=acarsitem.isuitem.AESID;
                //downlinkgroups.adownlinkbasicreportgroup.downlinkheader=downlinkheader;
                downlinkgroups.adownlinkbasicreportgroup.messagetype=(ADSDownlinkMessages)appmessage_bytes.at(i);
                downlinkgroups.adownlinkbasicreportgroup.latitude=latitude;
                downlinkgroups.adownlinkbasicreportgroup.longitude=longitude;
                downlinkgroups.adownlinkbasicreportgroup.altitude=altitude;
                downlinkgroups.adownlinkbasicreportgroup.time_stamp=time_stamp;
                downlinkgroups.adownlinkbasicreportgroup.FOM=FOM;
                downlinkgroups.adownlinkbasicreportgroup.valid=true;

                i+=11;//goto next message
                break;
            }
            case Noncompliance_Notification://not fully implimented.
            {
                if((i+6)>appmessage_bytes.size()){fail=true;break;}//sz check
                arincmessage->info+="Noncompliance_Notification";
                arincmessage->info+=((QString)" ADS Contract Request Number = %1. Not fully implimented\n").arg((uchar)appmessage_bytes.at(i-1+2));
                i+=6;//goto next message
                break;
            }
            case Cancel_Emergency_Mode:
            {
                arincmessage->info+="Cancel_Emergency_Mode\n";
                i+=1;//goto next message
                break;
            }
            case Airframe_Identification_Group://not implimented. can't be bothered its just the AES # and we already got that
            {
                if((i+4)>appmessage_bytes.size()){fail=true;break;}//sz check
                arincmessage->info+="Airframe_Identification. Not implimented\n";
                i+=4;//goto next message
                break;
            }
            case Intermediate_Projected_Intent_Group:
            {
                if((i+9)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Intermediate_Projected_Intent_Group";

                //Distance
                double distance=((double)extractqint32(appmessage_bytes,i-1+3,0,16,false))*distance_scaller;
                //True track
                bool truetrack_isvalid=false;
                if(((appmessage_bytes.at(i-1+4)>>7)&0x01)==0)truetrack_isvalid=true;
                double truetrack=((double)extractqint32(appmessage_bytes,i-1+5,3,12,true))*truetrack_scaller;//i think doc had an error with this so have done it the way I thought was correct
                if(truetrack<0)truetrack+=360.0;
                //Alt
                double altitude=((double)extractqint32(appmessage_bytes,i-1+7,3,16,true))*alt_scaller;
                //Projected Time
                double projected_time=((double)extractqint32(appmessage_bytes,i-1+9,5,14,false));

                if(truetrack_isvalid)arincmessage->info+=middlespacer+((QString)"Intermediate intent: Distance = %1 nm. True Track = %2 deg. Alt = %3 feet. Projected Time = %4\n").arg(distance).arg(qRound(truetrack)).arg(altitude).arg(QDateTime::fromTime_t(projected_time).toUTC().toString("hh:mm:ss"));
                else middlespacer+((QString)"Intermediate intent: Distance = %1 nm. Alt = %2 feet. Projected Time = %3\n").arg(distance).arg(altitude).arg(QDateTime::fromTime_t(projected_time).toUTC().toString("hh:mm:ss"));

                i+=9;//goto next message
                break;
            }
            case Fixed_Projected_Intent_Group:
            {
                if((i+10)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Fixed_Projected_Intent_Group";

                //Lat
                double latitude=((double)extractqint32(appmessage_bytes,i-1+4,3,21,true))*lat_scaller;
                //Long
                double longitude=((double)extractqint32(appmessage_bytes,i-1+7,6,21,true))*long_scaller;
                //Alt
                double altitude=((double)extractqint32(appmessage_bytes,i-1+9,6,16,true))*alt_scaller;
                //Projected Time
                double projected_time=((double)extractqint32(appmessage_bytes,i-1+10,0,14,false));

                arincmessage->info+=middlespacer+((QString)"Fixed intent: Lat = %1 Long = %2 Alt = %3 feet. Projected Time = %4\n").arg(latitude).arg(longitude).arg(altitude).arg(QDateTime::fromTime_t(projected_time).toUTC().toString("hh:mm:ss"));

                i+=10;//goto next message
                break;
            }
            default:
                arincmessage->info+=((QString)"Group %1 unknown. Can't continue\n").arg((uchar)appmessage_bytes.at(i));
                fail=true;//fail cos we have no idea what to do

                /* not sure what would be best. hmmm
                //lets say we dont trust this packet. In future if they change the format we will have to either add the group or remove this
                downlinkgroups.clear();
                arincmessage.clear();
                arincmessage.downlink=acarsitem.downlink;
                */


                break;
        }
    }
}

qint32 extractqint32(QByteArray &ba,int lsbyteoffset,int bitoffset,int numberofbits,bool issigned)
{
    numberofbits--;
    qint32 val=0;
    int shift=0;
    qint32 byte;
    int mask=(~((0x00FF)<<(8-bitoffset)))&0xFF;
    for(int i=lsbyteoffset;i>=0;i--)
    {
        if((i-1)>=0)byte=((ba.at(i)>>bitoffset)&mask)|((ba.at(i-1)<<(8-bitoffset))&~mask);
        else byte=((ba.at(i)>>bitoffset)&mask);
        byte&=0xFF;
        val|=(byte<<shift);
        shift+=8;
        if((shift>numberofbits)||(shift>24))break;
    }
    if(issigned)
    {
        if(((val>>numberofbits)&0x00000001)) val|=(0xFFFFFFFF<<(numberofbits+1));
        else val&=~(0xFFFFFFFF<<(numberofbits+1));
    }
    else val&=~(0xFFFFFFFF<<(numberofbits+1));
    return val;
}