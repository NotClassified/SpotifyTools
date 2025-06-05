#pragma once

#include <JuceHeader.h>
#include "Globals.h"
#include "JsonStream.h"


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::Component
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    struct Time
    {
        Time() {}
        Time(juce::String timeString)
        {
            if (!Time::isTextAValidTime(timeString))
                return;

            hour = timeString.substring(0, 2).getIntValue() % 24;
            minute = timeString.substring(3).getIntValue();
        }
        Time(std::tm* stdTime) : hour(stdTime->tm_hour), minute(stdTime->tm_min), second(stdTime->tm_sec) {}
        Time(time_t t) : hour(localtime(&t)->tm_hour), minute(localtime(&t)->tm_min), second(localtime(&t)->tm_sec) {}

        bool operator==(Time t) { return t.hour == hour && t.minute == minute; }

        juce::String toString() const
        {
            juce::String output;
            if (hour < 10)
                output += "0";
            output += juce::String(hour);
            output += ':';
            if (minute < 10)
                output += "0";
            output += juce::String(minute);

            return output;
        }

        void setHourFromPercentage(double hourPercent) { hour = (int)(hourPercent * 24) % 24; }
        void setMinuteFromPercentage(double minutePercent) { minute = (int)(minutePercent * 60) % 60; }

        int getAbsoluteMinutes() const { return hour * 60 + minute; }
        int getAbsoluteSeconds() const { return getAbsoluteMinutes() * 60 + second; }

        //format: 12:00
        inline static bool isTextAValidTime(juce::String text) { return text.length() == 5 && text[2] == ':'; }


        int hour = 0;
        int minute = 0;
        int second = 0;
    };

    const int PST_TO_GMT = -7 * 60 * 60;


    struct Track
    {
        Track() {}
        Track(const jString& artistName, const jString& albumName, const jString& songName, const jString& timePlayed)
            : artistName(artistName), albumName(albumName), songName(songName), timePlayed(timePlayed)
        {
        }

        bool isSameAlbum(Track* track)
        {
            return track->artistName == artistName && track->albumName == albumName;
        }

        jString artistName;
        jString albumName;
        jString songName;
        jString timePlayed;
    };

    vArray<Track*> playedTracks;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    juce::TextButton readJsonFromClipboardButton{ "Get Recently Played Albums" };
    juce::TextEditor resultsView;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
