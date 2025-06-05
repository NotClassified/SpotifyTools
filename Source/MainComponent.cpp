#include "MainComponent.h"
#include <chrono>

//==============================================================================
MainComponent::MainComponent()
{
    const auto timeNow = std::chrono::system_clock::now();
    juce::SystemClipboard::copyTextToClipboard(jString(timeNow.time_since_epoch().count() / 1000 + PST_TO_GMT));

    addAndMakeVisible(readJsonFromClipboardButton);
    readJsonFromClipboardButton.onClick = [=]
    {
        json::Stream stream(juce::SystemClipboard::getTextFromClipboard());
        if (stream.getStart().isNotValid())
            return;
        
        for (auto& item : stream["items"].getArray())
        {
            playedTracks.add(new Track(item["track"]["artists"].getArray()[0]["name"].getString(), 
                                       item["track"]["album"]["name"].getString(), 
                                       item["track"]["name"].getString(),
                                       item["played_at"].getString()));
        }

        //get next before time
        {
            //format: 
            //2025-05-22T18:31:32.751Z
            //012345678901234567890123
            //0         10        20  
            jString& timePlayedString = playedTracks.getLast()->timePlayed;
            tm lastTrackPlayTime = { 0 };
            lastTrackPlayTime.tm_year = timePlayedString.substring(0, 4).getIntValue() - 1900;
            lastTrackPlayTime.tm_mon = timePlayedString.substring(5, 7).getIntValue();
            lastTrackPlayTime.tm_mday = timePlayedString.substring(8, 10).getIntValue();
            lastTrackPlayTime.tm_hour = timePlayedString.substring(11, 13).getIntValue();
            lastTrackPlayTime.tm_min = timePlayedString.substring(14, 16).getIntValue();
            lastTrackPlayTime.tm_sec = timePlayedString.substring(17, 19).getIntValue();

            time_t timeSinceEpochGMT = mktime(&lastTrackPlayTime) + PST_TO_GMT;
            time_t timeSinceEpochMS = (timeSinceEpochGMT - 1) * 1000;
            juce::SystemClipboard::copyTextToClipboard(std::to_string(timeSinceEpochMS));
        }

        //jString nextUrl = stream["next"].getString();
        //int beforeKeyEndIndex = nextUrl.indexOf("before=") + 7;
        //jString timeStampString = nextUrl.substring(beforeKeyEndIndex, nextUrl.indexOf(beforeKeyEndIndex, "&"));
        //juce::int64 nextTimestampMS = timeStampString.getLargeIntValue() * 1000;
        //juce::SystemClipboard::copyTextToClipboard(jString(nextTimestampMS));

        vArray<Track*> playedAlbums;
        Track* previousTrack = nullptr;
        for (size_t i = 0; i < playedTracks.size(); i++)
        {
            if ( previousTrack != nullptr && playedTracks[i]->isSameAlbum(previousTrack) && (playedAlbums.size() == 0 || !playedAlbums.getLast()->isSameAlbum(playedTracks[i])) )
            {
                playedAlbums.add(playedTracks[i]);
            }
            previousTrack = playedTracks[i];
        }

        jString resultsString = "";
        for (auto& i : playedAlbums)
        {
            resultsString += i->artistName + " - " + i->albumName + "\n";
        }
        resultsView.setText(resultsString);
    };

    addAndMakeVisible(resultsView);
    resultsView.setMultiLine(true, false);

    setSize (600, 400);
}

MainComponent::~MainComponent()
{
    Bounds localBounds = getLocalBounds();

    readJsonFromClipboardButton.setBounds(localBounds.removeFromTop(25));
    resultsView.setBounds(localBounds);
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    Bounds localBounds = getLocalBounds();

    readJsonFromClipboardButton.setBounds(localBounds.removeFromTop(25));
    resultsView.setBounds(localBounds);
}
