#ifndef I3C_ANALYZER_SETTINGS
#define I3C_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class I3CAnalyzerSettings : public AnalyzerSettings
{
public:
	I3CAnalyzerSettings();
	virtual ~I3CAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mSdaChannel;
	Channel mSclChannel;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mSdaChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mSclChannelInterface;
};

#endif //I3C_ANALYZER_SETTINGS
