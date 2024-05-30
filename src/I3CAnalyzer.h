#ifndef I3C_ANALYZER_H
#define I3C_ANALYZER_H

#include <Analyzer.h>
#include "I3CAnalyzerResults.h"
#include "I3CSimulationDataGenerator.h"

typedef enum {event_none, event_rising_clock, event_falling_clock, event_start, event_stop} t_event;
typedef enum {state_idle, state_0, state_1, state_2, state_3, state_4, state_5, state_6} t_state;
typedef enum {mode_sdr, mode_ddr} t_mode;
typedef enum {protevent_start, protevent_restart, protevent_stop, protevent_data, protevent_entdaadata, protevent_hdr_restart, protevent_hdr_exit, protevent_targetreset, protevent_hdr_dataword, protevent_hdr_crcword} t_protevent;
typedef struct 
{
	uint32_t state;
	uint8_t  bitcounter;
	uint8_t dataword;
	uint8_t  ack_rising;
	uint8_t  ack_falling;
} t_decodestate_sdr;

typedef struct 
{
	uint32_t  state;
	uint8_t   bitcounter;
	uint32_t  dataword;
} t_decodestate_ddr;



class I3CAnalyzerSettings;
class ANALYZER_EXPORT I3CAnalyzer : public Analyzer2
{
public:
	I3CAnalyzer();
	virtual ~I3CAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();
	
private:
	void decodeI3C(void);
	void decodeevent(U64 s, BitState scl, BitState sda, t_event event);
	void protocolevent(U64 s, t_protevent event, U64 dataword, uint8_t ack_rising, uint8_t ack_falling);
	t_state mState;
	t_mode  mMode; // contains the current transfer mode (e.g. SDR, HDR-DDR, ...)
	
	// entdaa decoding related state information
	uint8_t mentdaa_detector;    // counter for entdaa detection state machine
	uint8_t mentdaa_bytecounter; // used as counter to check if 8 entdaa payload bytes have already been received
	
	// SDR decoding related state information
	t_decodestate_sdr mStateSdrDecode;
	t_decodestate_ddr mStateDdrDecode;
	bool mIsFirstByte;
	bool mIsRead; // fla
	U64 mPrevS;
	
	// HDR decoding state related information
	uint8_t menthdr_detector;
	
	// state information for decoding of HDR Exit, HDR Restart and Target reset pattern
	// Those special conditions are detected based on a counter mechanism:
	//   - falling SCL edges are counted while SCL is LOW
	//   - when SCL is high, the counter is reset to 0
	//   As soon as SCL gets high, the counter value is compared against a value
	//      2 -> HDR restart condition detected
	//      4 -> HDR exit condition detected
	//      7 -> Target reset condition dected
	uint8_t mSpecialConditionDetectionState;

protected: //vars
	std::auto_ptr< I3CAnalyzerSettings > mSettings;
	std::auto_ptr< I3CAnalyzerResults > mResults;
    AnalyzerChannelData* mSda;
    AnalyzerChannelData* mScl;

	I3CSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //I3C_ANALYZER_H
