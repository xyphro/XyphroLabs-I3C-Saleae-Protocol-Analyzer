#include "I3CAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


I3CAnalyzerSettings::I3CAnalyzerSettings() : mSdaChannel( UNDEFINED_CHANNEL ), mSclChannel( UNDEFINED_CHANNEL )
{
    mSdaChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mSdaChannelInterface->SetTitleAndTooltip( "SDA", "Serial Data Line" );
    mSdaChannelInterface->SetChannel( mSdaChannel );

    mSclChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mSclChannelInterface->SetTitleAndTooltip( "SCL", "Serial Clock Line" );
    mSclChannelInterface->SetChannel( mSclChannel );

    AddInterface( mSdaChannelInterface.get() );
    AddInterface( mSclChannelInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

    ClearChannels();
    AddChannel( mSdaChannel, "SDA", false );
    AddChannel( mSclChannel, "SCL", false );
}

I3CAnalyzerSettings::~I3CAnalyzerSettings()
{
}

bool I3CAnalyzerSettings::SetSettingsFromInterfaces()
{
    if( mSdaChannelInterface->GetChannel() == mSclChannelInterface->GetChannel() )
    {
        SetErrorText( "SDA and SCL can't be assigned to the same input." );
        return false;
    }

    mSdaChannel = mSdaChannelInterface->GetChannel();
    mSclChannel = mSclChannelInterface->GetChannel();

    ClearChannels();
    AddChannel( mSdaChannel, "SDA", true );
    AddChannel( mSclChannel, "SCL", true );

	return true;
}

void I3CAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mSdaChannelInterface->SetChannel( mSdaChannel );
    mSclChannelInterface->SetChannel( mSclChannel );
}

void I3CAnalyzerSettings::LoadSettings( const char* settings )
{
    SimpleArchive text_archive;
    text_archive.SetString( settings );

    const char* name_string; // the first thing in the archive is the name of the protocol analyzer that the data belongs to.
    text_archive >> &name_string;
    if( strcmp( name_string, "XyphroLabsI3CAnalyzer" ) != 0 )
        AnalyzerHelpers::Assert( "XyphroLabsI3CAnalyzer: Provided with a settings string that doesn't belong to us;" );

    text_archive >> mSdaChannel;
    text_archive >> mSclChannel;

    ClearChannels();
    AddChannel( mSdaChannel, "SDA", true );
    AddChannel( mSclChannel, "SCL", true );

    UpdateInterfacesFromSettings();
}

const char* I3CAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << "XyphroLabsI3CAnalyzer";
    text_archive << mSdaChannel;
    text_archive << mSclChannel;

    return SetReturnString( text_archive.GetString() );
}
