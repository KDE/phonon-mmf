/*  This file is part of the KDE project
    Copyright (C) 2006-2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef PHONON_XINE_XINESTREAM_H
#define PHONON_XINE_XINESTREAM_H

#include "audioport.h"
#include "audiopostlist.h"

#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QMultiMap>
#include <QtCore/QWaitCondition>
#include <QtCore/QTimer>

#include <Phonon/Global>

#include <xine.h>

#include <sys/time.h>
#include <time.h>

class KUrl;

namespace Phonon
{
namespace Xine
{
class VideoWidget;
class SeekCommandEvent;
class MediaObject;
class XineThread;

/**
 * \brief xine_stream_t wrapper that runs in its own thread.
 *
 * The xine_stream_t object is created as late as possible so that it doesn't have to be recreated
 * when an audio- or video_port is added.
 *
 * \author Matthias Kretz <kretz@kde.org>
 */
class XineStream : public QObject
{
    Q_OBJECT
    public:
        XineStream(XineThread *parent);
        ~XineStream();

        Phonon::State state() const { return m_state; }

        int totalTime() const;
        int remainingTime() const;
        int currentTime() const;
        bool hasVideo() const;
        bool isSeekable() const;
        void setVolume(int vol);

        void addAudioPostList(const AudioPostList &);
        void removeAudioPostList(const AudioPostList &);
        const QList<AudioPostList>& audioPostLists() { return m_audioPostLists; }

        void setVideoPort(VideoWidget *vwi);
        void setTickInterval(qint32 interval);
        void setPrefinishMark(qint32 time);

        //void needRewire(AudioPostList *postList);
        void setParam(int param, int value);
        void eventSend(xine_event_t *);
        void useGaplessPlayback(bool);
        void gaplessSwitchTo(const KUrl &url);
        void gaplessSwitchTo(const QByteArray &mrl);
        void closeBlocking();
        void aboutToDeleteVideoWidget();
        VideoWidget *videoWidget() const
        {
            if (m_newVideoPort) {
                return m_newVideoPort;
            }
            return m_videoPort;
        }

        void setError(Phonon::ErrorType, const QString &);
        QString errorString() const { return m_errorString; }
        Phonon::ErrorType errorType() const { return m_errorType; }

        int availableChapters() const { return m_availableChapters; }
        int availableAngles()   const { return m_availableAngles;   }
        int availableTitles()   const { return m_availableTitles;   }
        int currentChapter()    const { return m_currentChapter;    }
        int currentAngle()      const { return m_currentAngle;      }
        int currentTitle()      const { return m_currentTitle;      }

        enum StateForNewMrl {
            // no use: Loading, Error, Buffering
            StoppedState = Phonon::StoppedState,
            PlayingState = Phonon::PlayingState,
            PausedState = Phonon::PausedState,
            KeepState = 0xff
        };

        xine_post_out_t *audioSource() const { Q_ASSERT(m_stream); return xine_get_audio_source(m_stream); }

    public slots:
        void setUrl(const KUrl &url);
        void setMrl(const QByteArray &mrl, StateForNewMrl = StoppedState);
        void play();
        void pause();
        void stop();
        void seek(qint64 time);

        /**
         * all signals emitted from the xine thread
         */
    Q_SIGNALS:
        void finished();
        void stateChanged(Phonon::State newstate, Phonon::State oldstate);
        void metaDataChanged(const QMultiMap<QString, QString>&);
        void length(qint64);
        void seekDone();
        void needNextUrl();
        void tick(qint64);
        void prefinishMarkReached(qint32);
        void seekableChanged(bool);
        void hasVideoChanged(bool);
        void bufferStatus(int);

        void availableChaptersChanged(int);
        void chapterChanged(int);
        void availableAnglesChanged(int);
        void angleChanged(int);
        void availableTitlesChanged(int);
        void titleChanged(int);

    protected:
        bool event(QEvent *ev);
        void timerEvent(QTimerEvent *event);

    private slots:
        void getStartTime();
        void emitAboutToFinish();
        void emitTick();

    private:
        void getStreamInfo();
        bool xineOpen(Phonon::State);
        void updateMetaData();
        void rewireOutputPorts();
        bool createStream();
        void changeState(Phonon::State newstate);
        void emitAboutToFinishIn(int timeToAboutToFinishSignal);
        bool updateTime();
        void playbackFinished();
        void error(Phonon::ErrorType, const QString &);
        void internalPause();
        void internalPlay();

        xine_stream_t *m_stream;
        xine_event_queue_t *m_event_queue;

        QList<AudioPostList> m_audioPostLists;

        VideoWidget *m_videoPort;
        VideoWidget *m_newVideoPort;

        Phonon::State m_state;

        QMutex m_portMutex;
        mutable QMutex m_mutex;
        mutable QMutex m_streamInfoMutex;
        mutable QMutex m_updateTimeMutex;
        mutable QWaitCondition m_waitingForStreamInfo;
        QWaitCondition m_waitingForClose;
        QWaitCondition m_waitingForRewire;
        QMultiMap<QString, QString> m_metaDataMap;
        QByteArray m_mrl;
        QTimer *m_prefinishMarkTimer;
        struct timeval m_lastTimeUpdate;

        QString m_errorString;
        Phonon::ErrorType m_errorType;

        SeekCommandEvent *m_lastSeekCommand;
        qint32 m_prefinishMark;
        int m_volume;
        int m_startTime;
        int m_totalTime;
        int m_currentTime;
        int m_waitForPlayingTimerId;
        int m_availableTitles;
        int m_availableChapters;
        int m_availableAngles;
        int m_currentAngle;
        int m_currentTitle;
        int m_currentChapter;
        bool m_streamInfoReady : 1;
        bool m_hasVideo : 1;
        bool m_isSeekable : 1;
        bool m_rewireEventSent : 1;
        bool m_useGaplessPlayback : 1;
        bool m_prefinishMarkReachedNotEmitted : 1;
        bool m_ticking : 1;
        bool m_closing : 1;
        bool m_eventLoopReady : 1;
        QTimer m_tickTimer;
};

} // namespace Xine
} // namespace Phonon

#endif // PHONON_XINE_XINESTREAM_H
