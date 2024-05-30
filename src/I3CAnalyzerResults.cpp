#include "I3CAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "I3CAnalyzer.h"
#include "I3CAnalyzerSettings.h"
#include <iostream>
#include <sstream>
#include <fstream>

I3CAnalyzerResults::I3CAnalyzerResults( I3CAnalyzer* analyzer, I3CAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

I3CAnalyzerResults::~I3CAnalyzerResults()
{
}

void I3CAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	std::stringstream ss;
    char ack1[32], ack2[32];


	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	char number_str[128];
	
	if ( frame.mType == FRAMETYPE_ADDRESS )
	{
		if( ( frame.mFlags & FRAMEFLAGS_ACK_SENDER ) != 0 )
			snprintf( ack1, sizeof( ack1 ), "NAK" );
		else
			snprintf( ack1, sizeof( ack1 ), "ACK" );

		if( ( frame.mFlags & FRAMEFLAGS_ACK_RECEIVER ) != 0 )
			snprintf( ack2, sizeof( ack2 ), "ABORT" );
		else
			snprintf( ack2, sizeof( ack2 ), "CONT" );

		bool read = (frame.mFlags & FRAMEFLAGS_READ) != 0;
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
		if (read)
		{
			ss << "R[" << number_str << "]";
			AddResultString( ss.str().c_str() );
			ss.str( "" );

			ss << "Read [" << number_str << "]";
			AddResultString( ss.str().c_str() );
			ss.str( "" );

			ss << "Setup Read to [" << number_str << "] + " << ack1 << "+" << ack2;
			AddResultString( ss.str().c_str() );
			ss.str( "" );
		}
		else
		{
			ss << "W[" << number_str << "]";
			AddResultString( ss.str().c_str() );
			ss.str( "" );

			ss << "Write [" << number_str << "]";
			AddResultString( ss.str().c_str() );
			ss.str( "" );

			ss << "Setup Write to [" << number_str << "] + " << ack1 << "+" << ack2;
			AddResultString( ss.str().c_str() );
			ss.str( "" );
		}
	}
	else if ( (frame.mType == FRAMETYPE_RDATA) || (frame.mType == FRAMETYPE_WDATA) )
	{
		char ack1[32], ack2[32];
		if (frame.mType == FRAMETYPE_WDATA)
		{ // write byte (controller => target)
			if( ( frame.mFlags & FRAMEFLAGS_ACK_SENDER ) != 0 )
				snprintf( ack1, sizeof( ack1 ), "PAR1" );
			else
				snprintf( ack1, sizeof( ack1 ), "PAR0" );

			if( ( frame.mFlags & FRAMEFLAGS_ACK_RECEIVER ) != 0 ) // 2nd clock edge does not contain additional information as for reads
				snprintf( ack2, sizeof( ack2 ), "" );
			else
				snprintf( ack2, sizeof( ack2 ), "" );
		}
		else
		{ // read byte (target <= controller)
			if( ( frame.mFlags & FRAMEFLAGS_ACK_SENDER ) != 0 )
				snprintf( ack1, sizeof( ack1 ), "CONT" );
			else
				snprintf( ack1, sizeof( ack1 ), "ABORT" );

			if( ( frame.mFlags & FRAMEFLAGS_ACK_RECEIVER ) != 0 )
				snprintf( ack2, sizeof( ack2 ), "+CONT" );
			else
				snprintf( ack2, sizeof( ack2 ), "+ABORT" );
		}
			
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
		
		ss << number_str << " + " << ack1 << ack2;
		AddResultString( ss.str().c_str() );
		ss.str( "" );
		
		ss << number_str;
		AddResultString( ss.str().c_str() );
		ss.str( "" );
	}
	else if ( (frame.mType == FRAMETYPE_ENTDAA)  )
	{
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
		ss << "ENTDAA payload [" << number_str << "]";
		AddResultString( ss.str().c_str() );
		ss.str( "" );

		ss << "ENTDAA PL [" << number_str << "]";
		AddResultString( ss.str().c_str() );
		ss.str( "" );

		ss << number_str ;
		AddResultString( ss.str().c_str() );
		ss.str( "" );
	}
	else if ( (frame.mType == FRAMETYPE_DDRCMD)  )
	{
		char spream[6], sparity[6];
		uint8_t preamble = (frame.mData1>>18) & 0x3;
		uint8_t parity   = frame.mData1 & 0x3;
		switch (preamble)
		{
			case 0: strcpy(spream, "00"); break;
			case 1: strcpy(spream, "01"); break;
			case 2: strcpy(spream, "10"); break;
			default:
			case 3: strcpy(spream, "11"); break;
		}
		switch (parity)
		{
			case 0: strcpy(sparity, "00"); break;
			case 1: strcpy(sparity, "01"); break;
			case 2: strcpy(sparity, "10"); break;
			default:
			case 3: strcpy(sparity, "11"); break;
		}
		//(frame.mData1>>2) & 0xfffful
		AnalyzerHelpers::GetNumberString( (frame.mData1>>2)&0xfffful, display_base, 16, number_str, 128 );
		//AnalyzerHelpers::GetNumberString( 0x1234, display_base, 16,  number_str, 128 );
		ss << "CMD "<< "[" << spream << "]" << " [" << number_str << "]" << " [" << sparity << "]";
		AddResultString( ss.str().c_str() );
		ss.str( "" );

		ss << "[" << spream << "]" << " [" << number_str << "]" << " [" << sparity << "]";
		AddResultString( ss.str().c_str() );
		ss.str( "" );

		ss << "[" << number_str << "]";
		AddResultString( ss.str().c_str() );
		ss.str( "" );
	}
	else if ( (frame.mType == FRAMETYPE_DDRDATA)  )
	{
		char spream[6], sparity[6];
		uint8_t preamble = (frame.mData1>>18) & 0x3;
		uint8_t parity   = frame.mData1 & 0x3;
		switch (preamble)
		{
			case 0: strcpy(spream, "00"); break;
			case 1: strcpy(spream, "01"); break;
			case 2: strcpy(spream, "10"); break;
			default:
			case 3: strcpy(spream, "11"); break;
		}
		switch (parity)
		{
			case 0: strcpy(sparity, "00"); break;
			case 1: strcpy(sparity, "01"); break;
			case 2: strcpy(sparity, "10"); break;
			default:
			case 3: strcpy(sparity, "11"); break;
		}
		
		AnalyzerHelpers::GetNumberString( (frame.mData1>>2)&0xfffful, display_base, 16, number_str, 128 );
		ss << "DATA "<< "[" << spream << "]" << " [" << number_str << "]" << " [" << sparity << "]";
		AddResultString( ss.str().c_str() );
		ss.str( "" );

		ss << "[" << spream << "]" << " [" << number_str << "]" << " [" << sparity << "]";
		AddResultString( ss.str().c_str() );
		ss.str( "" );

		ss << "[" << number_str << "]";
		AddResultString( ss.str().c_str() );
		ss.str( "" );
	}
	else if ( (frame.mType == FRAMETYPE_DDRCRC)  )
	{
		char spream[6], tokenstr[6];
		uint8_t preamble = (frame.mData1>>18) & 0x3;
		uint8_t parity   = frame.mData1 & 0x3;
		switch (preamble)
		{
			case 0: strcpy(spream, "00"); break;
			case 1: strcpy(spream, "01"); break;
			case 2: strcpy(spream, "10"); break;
			default:
			case 3: strcpy(spream, "11"); break;
		}
		AnalyzerHelpers::GetNumberString( (frame.mData1>>5)&0xful, display_base, 4, tokenstr, 128 );
		AnalyzerHelpers::GetNumberString( (frame.mData1>>2)&0xfffful, display_base, 5, number_str, 128 );
		ss << "CRC "<< "[" << spream << "]" << " [" << tokenstr << "]" << " [" << number_str << "]";
		AddResultString( ss.str().c_str() );
		ss.str( "" );

		ss << "[" << spream << "]" << " [" << tokenstr << "]" << " [" << number_str << "]";
		AddResultString( ss.str().c_str() );
		ss.str( "" );

		ss << "[" << number_str << "]";
		AddResultString( ss.str().c_str() );
		ss.str( "" );
	}	
	else
	{
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
		ss << number_str ;
		AddResultString( ss.str().c_str() );
	}
}

void I3CAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Value" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

		file_stream << time_str << "," << number_str << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void I3CAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
	Frame frame = GetFrame( frame_index );
	ClearTabularText();

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	AddTabularText( number_str );
#endif
}

void I3CAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	//not supported

}

void I3CAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	//not supported
}