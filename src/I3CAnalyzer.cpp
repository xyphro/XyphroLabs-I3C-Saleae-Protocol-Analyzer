#include "I3CAnalyzer.h"
#include "I3CAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

I3CAnalyzer::I3CAnalyzer()
:	Analyzer2(),  
	mSettings( new I3CAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
    UseFrameV2();
}

I3CAnalyzer::~I3CAnalyzer()
{
	KillThread();
}

void I3CAnalyzer::SetupResults()
{
	mResults.reset( new I3CAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mSdaChannel );
}

void I3CAnalyzer::WorkerThread()
{
    mSampleRateHz = GetSampleRate();
	mState = state_idle;
	mMode = mode_sdr;
	mStateSdrDecode.state = 0;
	mentdaa_detector = 0;
	mIsFirstByte = false;
	mIsRead = false;
	mPrevS = 0;
	mSpecialConditionDetectionState = 0;
	menthdr_detector = 0;
	
	mStateDdrDecode.state = 0;
	mStateDdrDecode.dataword = 0;
	mStateDdrDecode.bitcounter = 0;

    mSda = GetAnalyzerChannelData( mSettings->mSdaChannel );
    mScl = GetAnalyzerChannelData( mSettings->mSclChannel );

    for( ;; )
    {
		decodeI3C();
		mScl->AdvanceToNextEdge(); // posedge
		ReportProgress( mScl->GetSampleNumber() );

        CheckIfThreadShouldExit();
    }
}


void I3CAnalyzer::decodeI3C(void)
{
	BitState scl, sda, old_scl, old_sda;
	U64 s1, s2, s;
	t_event event;
	
	old_sda = BIT_HIGH;
	old_scl = BIT_HIGH;
	for( ;; )
	{
		// advance to next place where either scl or sda changes
		s1 = mScl->GetSampleOfNextEdge();
		s2 = mSda->GetSampleOfNextEdge();
		
		if (s1 < s2)
			s = s1;
		else
			s = s2;
		mScl->AdvanceToAbsPosition(s);
		mSda->AdvanceToAbsPosition(s);
		scl = mScl->GetBitState();
		sda = mSda->GetBitState();
		
		// Identify important bus conditions => check what happened
		// note: The order of condition checks is done like this on purpose to prioritize CLK edge changes
		event = event_none;
		if ( (old_scl == BIT_LOW) && (scl == BIT_HIGH) )
			event = event_rising_clock;
		else if ( (old_scl == BIT_HIGH) && (scl == BIT_LOW) )
			event = event_falling_clock;
		else if ( (scl == BIT_HIGH) && (old_sda == BIT_LOW) && (sda == BIT_HIGH) )
			event = event_stop;
		else if ( (scl == BIT_HIGH) && (old_sda == BIT_HIGH) && (sda == BIT_LOW) )
			event = event_start;
			
		// Check for HDR Exit, HDR Restart or TARGET Reset pattern
		if ( (scl == BIT_LOW) && (old_sda == BIT_HIGH) && (sda == BIT_LOW) )
		{
			mSpecialConditionDetectionState++;
			//mResults->AddMarker( s, AnalyzerResults::One, mSettings->mSdaChannel );
		} 
		if (event == event_rising_clock)
		{
			switch (mSpecialConditionDetectionState)
			{
				case 2: // HDR Restart
				case 3: // HDR Restart with SDA=HIGH setup
					protocolevent(s, protevent_hdr_restart, 0, 0, 0);
					break;
				case 4: // HDR Exit
				case 5: // HDR Exit with SDA=HIGH setup
					protocolevent(s, protevent_hdr_exit, 0, 0, 0);
					break;
				case 7: // HDR Target reset
				case 8: // HDR Target reset with SDA=HIGH setup
					protocolevent(s, protevent_targetreset, 0, 0, 0);
					break;
				default: // invalid
					break;
			}
			mSpecialConditionDetectionState= 0; // reset whenever scl is high
		}
			
		// Interpret events
		if (event != event_none)
			decodeevent(s, scl, sda, event);
			
		old_scl = scl;
		old_sda = sda;
	}
}


// ENTDAA payload detection looks for those order of states:
// ---------------------------------------------------------
// 1.) Start or restart
// 2.) 0x7e write with ACK +CONT (LOW LOW)
// 3.) received_0x07
// 3.) restart
// 4.) 0x7e with read and ACK +  CONT (low low)    -----> START of entdaa mode
// 5.) 64 clock edges have payload data			   <----- END of ENTDAA decoding mode
// 6.) then 9 clock cycles					       <----- SLAVE ADDRESS with 9th BIT

// ENTHDR payload detection looks for those order of states:
// ---------------------------------------------------------
// 1.) Start or restart
// 2.) 0x7e write with ACK + CONT (LOW LOW)
// 3.) received any of:
//         0x20 (HDR mode 0 - HDR-DDR [part of basic spec]) 
//         0x21 (HDR mode 1 - HDR-TSP)
//         0x22 (HDR mode 2 - HDR-TSL)
//         0x23 (HDR mode 3 - HDR-BT  [part of basic spec])
// 4.) Decoding will stay in HDR mode until HDR EXIT condition is detected

// low level events to protocol level decoder
void I3CAnalyzer::protocolevent(U64 s, t_protevent event, U64 dataword, uint8_t ack_rising, uint8_t ack_falling)
{
	FrameV2 framev2;
	
	uint32_t prevmenthdr_detector = menthdr_detector;
	
	switch (event)
	{
		case protevent_hdr_restart:
			mResults->AddMarker( s, AnalyzerResults::Start, mSettings->mSdaChannel );
			mResults->AddFrameV2( framev2, "HDR_restart", s, s + 1 );
			mIsFirstByte = true;
			break;
		case protevent_hdr_exit:
			mResults->AddMarker( s, AnalyzerResults::Stop, mSettings->mSdaChannel );
			mResults->AddFrameV2( framev2, "HDR_exit", s, s + 1 );
			mMode = mode_sdr; // back to SDR decoding mode
			mStateSdrDecode.state = 0;
			mentdaa_detector = 0;
			menthdr_detector = 0;
			break;

		case protevent_targetreset:
			mResults->AddMarker( s, AnalyzerResults::X, mSettings->mSdaChannel );
			mResults->AddFrameV2( framev2, "Target_reset", s, s + 1 );
			mMode = mode_sdr; // back to SDR decoding mode
			mStateSdrDecode.state = 0;
			mentdaa_detector = 0;
			menthdr_detector = 0;
			break;

		case protevent_start:
			mResults->AddMarker( s, AnalyzerResults::Start, mSettings->mSdaChannel );
			mIsFirstByte = true;
			mResults->AddFrameV2( framev2, "start", s, s + 1 );
			// handle entDaa payload detection state machine
			mentdaa_detector = 1;
			menthdr_detector = 1;
			break;
		case protevent_restart:
			mIsFirstByte = true;
			mResults->AddMarker( s, AnalyzerResults::Square, mSettings->mSdaChannel );
			mResults->AddFrameV2( framev2, "restart", s, s + 1 );
			if (mentdaa_detector == 3)
				mentdaa_detector = 4;
			else
				mentdaa_detector = 1;
			menthdr_detector = 1;
			break;
		case protevent_stop:
			mResults->AddMarker( s, AnalyzerResults::Stop, mSettings->mSdaChannel );
			mResults->CommitPacketAndStartNewPacket();
			mResults->CommitResults();
			mResults->AddFrameV2( framev2, "stop", s, s + 1 );
			mentdaa_detector = 0;
			menthdr_detector = 0;
			break;
		case protevent_entdaadata:
			{
				mentdaa_bytecounter++;
				Frame frame;
				frame.mStartingSampleInclusive = mPrevS;
				frame.mEndingSampleInclusive = s;
				frame.mType = FRAMETYPE_UNDEF;
				frame.mFlags  = 0;
				char* framev2Type = "entdaaPL";
				framev2.AddByte( "address", (U8)dataword);
				frame.mData1 = U8( dataword);
				frame.mType = FRAMETYPE_ENTDAA;
				mResults->AddFrame( frame );
				mResults->AddFrameV2( framev2, framev2Type, mPrevS, s );
				
				mResults->CommitResults();
				
				if (mentdaa_bytecounter == 8) // all 8 ENTDAA payload bytes received?
					mentdaa_detector = 0;
			}
			break;
		case protevent_data:
			{
				Frame frame;
				frame.mStartingSampleInclusive = mPrevS;
				frame.mEndingSampleInclusive = s;
				frame.mType = FRAMETYPE_UNDEF;
				frame.mFlags  = 0;
				
				// enthdr detector
				if      ( (menthdr_detector == 1) && (dataword == (0x7eul<<1)) && (ack_rising == 0) )
					menthdr_detector = 2;
				else if ( (menthdr_detector == 2) && (ack_rising == 0) &&
						  (dataword >= 0x20ul) && (dataword <= 0x23ul) )
				{
					mMode = mode_ddr;
					mStateSdrDecode.state = 0;
					menthdr_detector = 0;
				}
				// entdaa detector
				char* framev2Type = nullptr;			
				if      ( (mentdaa_detector == 1) && (dataword == (0x7eul<<1)) && (ack_rising == 0) )
					mentdaa_detector = 2;
				else if ( (mentdaa_detector == 2) && (dataword == (0x07ul)) && (ack_rising == 0) )
					mentdaa_detector = 3;
				else if ( (mentdaa_detector == 4) && (dataword == ((0x7eul<<1) | 1)) && (ack_rising == 0) )
				{
					mentdaa_detector = 5;			
					mentdaa_bytecounter = 0;
				}
				else if (mentdaa_detector >= 5)
					; // do purposefully nothing
				else
					mentdaa_detector = 0;
				
				if (mIsFirstByte) // this is set in case this is an address byte
				{
					mIsRead = (dataword & 1) != 0;
					framev2Type = "address";
					framev2.AddByte( "address", (U8)dataword >> 1);
					framev2.AddBoolean( "read", mIsRead);
					frame.mData1 = U8( dataword >> 1 );
					mIsFirstByte = false;
					frame.mType = FRAMETYPE_ADDRESS;
					if (mIsRead)
						frame.mFlags |= FRAMEFLAGS_READ;
				}
				else
				{
					framev2Type = "data";
					framev2.AddByte( "data", (U8)dataword );
					frame.mData1 = U8( dataword);
					if (mIsRead)
						frame.mType = FRAMETYPE_RDATA;
					else
						frame.mType = FRAMETYPE_WDATA;
				}
				if (ack_rising != 0)
					frame.mFlags  |= FRAMEFLAGS_ACK_SENDER;
				if (ack_falling != 0)
					frame.mFlags  |= FRAMEFLAGS_ACK_RECEIVER;
				framev2.AddBoolean( "ack", ack_rising == 0 );
				
				mResults->AddFrame( frame );
				mResults->AddFrameV2( framev2, framev2Type, mPrevS, s );
					
				mResults->CommitResults();
				// set flag that next dataword is first one when ENTHDR was received
				if (mMode == mode_ddr)				
					mIsFirstByte = true;
				break;
			}
		case protevent_hdr_dataword:
			{
				Frame frame;
				frame.mStartingSampleInclusive = mPrevS;
				frame.mEndingSampleInclusive = s;
				frame.mType = FRAMETYPE_DDRDATA;
				frame.mFlags  = 0;

				char* framev2Type = nullptr;
				if (mIsFirstByte)
				{
					frame.mType = FRAMETYPE_DDRCMD;
					framev2Type = "DDR_CMD";
					framev2.AddInteger( "address&offset", (dataword>>2)& 0xfffful);
					mIsFirstByte = false;
				}
				else
				{
					frame.mType = FRAMETYPE_DDRDATA;
					framev2Type = "DDR_DATA";
					framev2.AddInteger( "data", (dataword>>2)& 0xfffful);
				}
				frame.mData1 = dataword;
				framev2.AddByte( "preamble", (U8)((dataword >> 18) & 0x03) );
				framev2.AddByte( "parity", (U8)(dataword & 0x03) );
				
				
				mResults->AddFrame( frame );
				mResults->AddFrameV2( framev2, framev2Type, mPrevS, s );
					
				mResults->CommitResults();
				break;
			}
		case protevent_hdr_crcword:
			{
				Frame frame;
				frame.mStartingSampleInclusive = mPrevS;
				frame.mEndingSampleInclusive = s;
				frame.mType = FRAMETYPE_DDRCRC;
				frame.mFlags  = 0;

				char* framev2Type = nullptr;
				framev2Type = "DDR_crc";
				framev2.AddByte( "preamble", (U8)((dataword >> 9) & 0x03) );
				framev2.AddInteger( "token", (dataword>>5)& 0xful);
				framev2.AddInteger( "crc", (dataword>>0)& 0x1ful);
				frame.mData1 = dataword;
				
				mResults->AddFrame( frame );
				mResults->AddFrameV2( framev2, framev2Type, mPrevS, s );
					
				mResults->CommitResults();
				break;
			}
			
	}
	mPrevS = s;
	
	if (prevmenthdr_detector != menthdr_detector)
	{
		char str[16];
		//sprintf(str, "m%d", menthdr_detector);
		//mResults->AddFrameV2( framev2, str, s, s + 1 );
	}
}

void I3CAnalyzer::decodeevent(U64 s, BitState scl, BitState sda, t_event event)
{

	if (mMode == mode_sdr)
	{
		switch (mStateSdrDecode.state)
		{
			case 0: // wait for start
				if (event == event_start)
				{
					mStateSdrDecode.state++;
					mStateSdrDecode.bitcounter = 0;
					mStateSdrDecode.dataword = 0;

					protocolevent(s, protevent_start, 0, 0, 0);
				}
				if (event == event_stop)
				{
					protocolevent(s, protevent_stop, 0, 0, 0);
				}
				break;
			case 1: // data bit receiver
				if (mentdaa_detector == 5) // check if entdaa is detected, then decode the bits different
				{
					if (event == event_rising_clock)
					{
						mStateSdrDecode.dataword = mStateSdrDecode.dataword & ~(1<<(7-mStateSdrDecode.bitcounter));
						if (sda == BIT_HIGH)
							mStateSdrDecode.dataword = mStateSdrDecode.dataword | (1<<(7-mStateSdrDecode.bitcounter));
						
						mStateSdrDecode.bitcounter++;
						mResults->AddMarker( s, AnalyzerResults::UpArrow, mSettings->mSclChannel );
					}
					if (mStateSdrDecode.bitcounter == 8)
					{
						protocolevent(s, protevent_entdaadata, mStateSdrDecode.dataword, 0, 0);
						mStateSdrDecode.bitcounter = 0;
					}
				}
				else
				{
					if (event == event_rising_clock)
					{
						mStateSdrDecode.dataword = mStateSdrDecode.dataword & ~(1<<(7-mStateSdrDecode.bitcounter));
						if (sda == BIT_HIGH)
							mStateSdrDecode.dataword = mStateSdrDecode.dataword | (1<<(7-mStateSdrDecode.bitcounter));
						
						mStateSdrDecode.bitcounter++;
						mResults->AddMarker( s, AnalyzerResults::UpArrow, mSettings->mSclChannel );
					} 
					else if ( (event == event_falling_clock) && (mStateSdrDecode.bitcounter == 8))
					{
						mStateSdrDecode.state++;
						mStateSdrDecode.bitcounter = 0;
					}
					else// if (mStateSdrDec.Decode.bitcounter == 0) // only check for start/stop conditions on bit 0.
					{
						if (event == event_start) // restart
						{
							protocolevent(s, protevent_restart, 0, 0, 0);
							mStateSdrDecode.bitcounter = 0;						
						}
						else if (event == event_stop)
						{
							protocolevent(s, protevent_stop, 0, 0, 0);
							mStateSdrDecode.state = 0; // back to idle = wait for real start condition
						}
					}
				}
				break;
			case 2: // check ACK rising and falling edge state (bit 9)
				if (event == event_rising_clock)
				{
					mStateSdrDecode.ack_rising = (sda == BIT_HIGH) ? 1 : 0;
					mResults->AddMarker( s, AnalyzerResults::UpArrow, mSettings->mSclChannel );
				}
				else if (event == event_falling_clock)
				{
					mStateSdrDecode.ack_falling = (sda == BIT_HIGH) ? 1 : 0;
					// TODO: signal to prot decode					
					protocolevent(s, protevent_data, mStateSdrDecode.dataword, mStateSdrDecode.ack_rising, mStateSdrDecode.ack_falling);
					mStateSdrDecode.state = 1;
					mResults->AddMarker( s, AnalyzerResults::DownArrow, mSettings->mSclChannel );
				}
				break;
			default:
				mStateSdrDecode.state = 0;
				break;
		}
	} // end of sdr mode SM
	else if (mMode == mode_ddr)
	{
		switch (mStateDdrDecode.state)
		{
			case 0:
				if ( (event == event_rising_clock) || (event == event_falling_clock) )
				{
					mStateDdrDecode.dataword = mStateDdrDecode.dataword & ~(1ul<<(19-mStateDdrDecode.bitcounter));
					if (sda == BIT_HIGH)
						mStateDdrDecode.dataword = mStateDdrDecode.dataword | (1ul<<(19-mStateDdrDecode.bitcounter));
					
					mStateDdrDecode.bitcounter++;
					if (sda == BIT_LOW)
						mResults->AddMarker( s, AnalyzerResults::Zero, mSettings->mSdaChannel );
					else
						mResults->AddMarker( s, AnalyzerResults::One, mSettings->mSdaChannel );
					if (mStateDdrDecode.bitcounter == 20)
					{
						protocolevent(s, protevent_hdr_dataword, mStateDdrDecode.dataword, 0, 0);
						mStateDdrDecode.bitcounter = 0;
					} else if ( (mStateDdrDecode.bitcounter == 12) && ((mStateDdrDecode.dataword>>18) == 1) && (!mIsFirstByte))
					{
						protocolevent(s, protevent_hdr_crcword, mStateDdrDecode.dataword>>9, 0, 0);
						mStateDdrDecode.bitcounter = 0;
					}
				}
		}
	} // end of ddr mode SM
	ReportProgress(s);
}


bool I3CAnalyzer::NeedsRerun()
{
	return false;
}

U32 I3CAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 I3CAnalyzer::GetMinimumSampleRateHz()
{
    return 2000000;
}

const char* I3CAnalyzer::GetAnalyzerName() const
{
	return "I3C (XyphroLabs)";
}

const char* GetAnalyzerName()
{
	return "I3C (XyphroLabs)";
}

Analyzer* CreateAnalyzer()
{
	return new I3CAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}