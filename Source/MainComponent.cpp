#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    juce::SystemClipboard::copyTextToClipboard(jString(getCurrentTimeSinceEpoch()));

    addAndMakeVisible(showRecentAlbumsButton);
    showRecentAlbumsButton.onClick = [=]
    {
        playedTracks.add(readTracks(juce::SystemClipboard::getTextFromClipboard()));

        //get next before time
        {
            //format: 
            //2025-05-22T18:31:32.751Z
            //012345678901234567890123
            //0         10        20  
            jString& timePlayedString = playedTracks.getLast()->timePlayed;
            tm lastTrackPlayTime = { 0 };
            lastTrackPlayTime.tm_year = timePlayedString.substring(0, 4).getIntValue() - 1900;
            lastTrackPlayTime.tm_mon = timePlayedString.substring(5, 7).getIntValue() - 1;
            lastTrackPlayTime.tm_mday = timePlayedString.substring(8, 10).getIntValue();
            lastTrackPlayTime.tm_min = timePlayedString.substring(14, 16).getIntValue();
            lastTrackPlayTime.tm_sec = timePlayedString.substring(17, 19).getIntValue();

            lastTrackPlayTime.tm_hour = 0;
            mktime(&lastTrackPlayTime);
            lastTrackPlayTime.tm_hour = timePlayedString.substring(11, 13).getIntValue(); //override what mktime() does since it'll change the hour if the DST is wrong

            time_t timeSinceEpochGMT = mktime(&lastTrackPlayTime);
            time_t timeSinceEpochMS = timeSinceEpochGMT * 1000 + PST_TO_GMT;
            juce::SystemClipboard::copyTextToClipboard(std::to_string(timeSinceEpochMS));
        }

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
            resultsString += i->artistName + " - " + i->albumName + " - " + i->timePlayed + "\n";
        }
        resultsView.setText(resultsString);
    };

    addAndMakeVisible(showAllRecentTracksButton);
    showAllRecentTracksButton.onClick = [=]
    {
        jString resultsString = "";
        for (auto& i : readTracks(juce::SystemClipboard::getTextFromClipboard()))
        {
            resultsString += i->artistName + " - " + i->albumName + " - " + i->timePlayed + "\n";
        }
        resultsView.setText(resultsString);
    };

    addAndMakeVisible(resultsView);
    resultsView.setMultiLine(true, false);

    setSize (600, 400);
}

MainComponent::~MainComponent()
{
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

    showRecentAlbumsButton.setBounds(localBounds.removeFromTop(25));
    showAllRecentTracksButton.setBounds(localBounds.removeFromTop(25));
    resultsView.setBounds(localBounds);
}
