#ifndef I3C_SIMULATION_DATA_GENERATOR
#define I3C_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class I3CAnalyzerSettings;

class I3CSimulationDataGenerator
{
public:
	I3CSimulationDataGenerator();
	~I3CSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, I3CAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	I3CAnalyzerSettings* mSettings;
	//U32 mSimulationSampleRateHz;

//protected:


};
#endif //I3C_SIMULATION_DATA_GENERATOR