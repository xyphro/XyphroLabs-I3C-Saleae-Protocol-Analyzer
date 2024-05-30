#ifndef I3C_ANALYZER_RESULTS
#define I3C_ANALYZER_RESULTS

#include <AnalyzerResults.h>

#define FRAMETYPE_UNDEF    (0u)
#define FRAMETYPE_ADDRESS  (1u)
#define FRAMETYPE_RDATA    (2u)
#define FRAMETYPE_WDATA    (3u)
#define FRAMETYPE_ENTDAA   (4u)
#define FRAMETYPE_DDRDATA  (5u)
#define FRAMETYPE_DDRCMD   (6u)
#define FRAMETYPE_DDRCRC   (7u)

#define FRAMEFLAGS_ACK_SENDER   (1u)
#define FRAMEFLAGS_ACK_RECEIVER (2u)

#define FRAMEFLAGS_READ         (4u)

class I3CAnalyzer;
class I3CAnalyzerSettings;

class I3CAnalyzerResults : public AnalyzerResults
{
public:
	I3CAnalyzerResults( I3CAnalyzer* analyzer, I3CAnalyzerSettings* settings );
	virtual ~I3CAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	I3CAnalyzerSettings* mSettings;
	I3CAnalyzer* mAnalyzer;
};

#endif //I3C_ANALYZER_RESULTS
