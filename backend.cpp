/* This file is part of the KDE project
 Copyright (C) 2006 Tim Beaulen <tbscope@gmail.com>
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

#include "backend.h"
#include "mediaobject.h"
#include "effect.h"
#include "events.h"
#include "audiooutput.h"
#include "audiodataoutput.h"
#include "visualization.h"
#include "volumefadereffect.h"
#include "videodataoutput.h"
#include "videowidget.h"
#include "wirecall.h"
#include "xinethread.h"

#include <kdebug.h>
#include <klocale.h>
#include <kgenericfactory.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QSet>
#include <QtCore/QVariant>

#include <phonon/audiodevice.h>
#include <phonon/audiodeviceenumerator.h>

extern "C" {
#include <xine/xine_plugin.h>
#include "sinknode.h"
#include "sourcenode.h"
extern plugin_info_t phonon_xine_plugin_info[];
}

typedef KGenericFactory<Phonon::Xine::Backend> XineBackendFactory;
K_EXPORT_COMPONENT_FACTORY(phonon_xine, XineBackendFactory("xinebackend"))

namespace Phonon
{
namespace Xine
{

Backend::Backend( QObject* parent, const QStringList& )
	: QObject( parent )
{
    setProperty("identifier",     QLatin1String("phonon_xine"));
    setProperty("backendName",    QLatin1String("Xine"));
    setProperty("backendComment", i18n("Phonon Xine Backend"));
    setProperty("backendVersion", QLatin1String("0.1"));
    setProperty("backendIcon",    QLatin1String("phonon-xine"));
    setProperty("backendWebsite", QLatin1String("http://multimedia.kde.org/"));

    new XineEngine(XineBackendFactory::componentData().config());
	char configfile[2048];

    xine_engine_set_param(XineEngine::xine(), XINE_ENGINE_PARAM_VERBOSITY, 99);
	sprintf(configfile, "%s%s", xine_get_homedir(), "/.xine/config");
	xine_config_load( XineEngine::xine(), configfile );
	xine_init( XineEngine::xine() );

	kDebug( 610 ) << "Using Xine version " << xine_get_version_string() << endl;

    connect(XineEngine::sender(), SIGNAL(objectDescriptionChanged(ObjectDescriptionType)),
            SIGNAL(objectDescriptionChanged(ObjectDescriptionType)));

    xine_register_plugins(XineEngine::xine(), phonon_xine_plugin_info);
}

Backend::~Backend()
{
	delete XineEngine::self();
}

QObject *Backend::createObject(BackendInterface::Class c, QObject *parent, const QList<QVariant> &args)
{
    switch (c) {
        case MediaObjectClass:
            return new MediaObject(parent);
        case VolumeFaderEffectClass:
            return new VolumeFaderEffect(parent);
        case AudioOutputClass:
            return new AudioOutput(parent);
        case AudioDataOutputClass:
            return new AudioDataOutput(parent);
        case VisualizationClass:
            return new Visualization(parent);
        case VideoDataOutputClass:
            return new VideoDataOutput(parent);
    case EffectClass:
        Q_ASSERT(args.size() == 1);
        return new Effect(args[0].toInt(), parent);
    case VideoWidgetClass:
        {
            VideoWidget *vw = new VideoWidget(qobject_cast<QWidget *>(parent));
            if (vw->isValid()) {
                return vw;
            }
            delete vw;
            return 0;
        }
    }
    return 0;
}

bool Backend::supportsVideo() const
{
	return true;
}

bool Backend::supportsOSD() const
{
	return true;
}

bool Backend::supportsFourcc( quint32 fourcc ) const
{
	switch( fourcc )
	{
		case 0x00000000:
			return true;
		default:
			return false;
	}
}

bool Backend::supportsSubtitles() const
{
	return true;
}

QStringList Backend::availableMimeTypes() const
{
	if( m_supportedMimeTypes.isEmpty() )
	{
		char* mimeTypes_c = xine_get_mime_types( XineEngine::xine() );
		QString mimeTypes( mimeTypes_c );
		free( mimeTypes_c );
		QStringList lstMimeTypes = mimeTypes.split( ";", QString::SkipEmptyParts );
		foreach( QString mimeType, lstMimeTypes )
			m_supportedMimeTypes << mimeType.left( mimeType.indexOf( ':' ) ).trimmed();
		if( m_supportedMimeTypes.contains( "application/ogg" ) )
			m_supportedMimeTypes << QLatin1String( "audio/x-vorbis+ogg" ) << QLatin1String( "application/ogg" );
	}

	return m_supportedMimeTypes;
}

QSet<int> Backend::objectDescriptionIndexes( ObjectDescriptionType type ) const
{
	QSet<int> set;
	switch( type )
	{
		case Phonon::AudioOutputDeviceType:
            return XineEngine::audioOutputIndexes();
		case Phonon::AudioCaptureDeviceType:
            {
                QList<AudioDevice> devlist = AudioDeviceEnumerator::availableCaptureDevices();
                foreach (AudioDevice dev, devlist) {
                    set << dev.index();
                }
            }
			break;
		case Phonon::VideoOutputDeviceType:
			{
				const char* const* outputPlugins = xine_list_video_output_plugins( XineEngine::xine() );
				for( int i = 0; outputPlugins[i]; ++i )
					set << 40000 + i;
				break;
			}
		case Phonon::VideoCaptureDeviceType:
			set << 30000 << 30001;
			break;
		case Phonon::VisualizationType:
			break;
		case Phonon::AudioCodecType:
			break;
		case Phonon::VideoCodecType:
			break;
		case Phonon::ContainerFormatType:
			break;
		case Phonon::EffectType:
			{
				const char* const* postPlugins = xine_list_post_plugins_typed( XineEngine::xine(), XINE_POST_TYPE_AUDIO_FILTER );
				for( int i = 0; postPlugins[i]; ++i )
					set << 0x7F000000 + i;
                const char *const *postVPlugins = xine_list_post_plugins_typed( XineEngine::xine(), XINE_POST_TYPE_VIDEO_FILTER );
                for (int i = 0; postVPlugins[i]; ++i) {
					set << 0x7E000000 + i;
                }
				break;
			}
	}
	return set;
}

QHash<QByteArray, QVariant> Backend::objectDescriptionProperties(ObjectDescriptionType type, int index) const
{
    //kDebug(610) << k_funcinfo << type << index << endl;
    QHash<QByteArray, QVariant> ret;
    switch (type) {
        case Phonon::AudioOutputDeviceType:
            {
                ret.insert("name", XineEngine::audioOutputName(index));
                ret.insert("description", XineEngine::audioOutputDescription(index));
                QString icon = XineEngine::audioOutputIcon(index);
                if (!icon.isEmpty()) {
                    ret.insert("icon", icon);
                }
                ret.insert("available", XineEngine::audioOutputAvailable(index));
            }
            break;
        case Phonon::AudioCaptureDeviceType:
            {
                QList<AudioDevice> devlist = AudioDeviceEnumerator::availableCaptureDevices();
                foreach (AudioDevice dev, devlist) {
                    if (dev.index() == index) {
                        ret.insert("name", dev.cardName());
                        switch (dev.driver()) {
                            case Solid::AudioInterface::Alsa:
                                ret.insert("description", i18n("ALSA Capture Device"));
                                break;
                            case Solid::AudioInterface::OpenSoundSystem:
                                ret.insert("description", i18n("OSS Capture Device"));
                                break;
                            case Solid::AudioInterface::UnknownAudioDriver:
                                break;
                        }
                        ret.insert("icon", dev.iconName());
                        ret.insert("available", dev.isAvailable());
                        break;
                    }
                }
            }
            switch (index) {
                case 20000:
                    ret.insert("name", QLatin1String("Soundcard"));
                    break;
                case 20001:
                    ret.insert("name", QLatin1String("DV"));
                    break;
            }
            //kDebug(610) << ret["name"] << endl;
            break;
        case Phonon::VideoOutputDeviceType:
            {
                const char *const *outputPlugins = xine_list_video_output_plugins(XineEngine::xine());
                for (int i = 0; outputPlugins[i]; ++i) {
                    if (40000 + i == index) {
                        ret.insert("name", QLatin1String(outputPlugins[i]));
                        ret.insert("description", "");
                        // description should be the result of the following call, but it crashes.
                        // It looks like libxine initializes the plugin even when we just want the description...
                        //QLatin1String(xine_get_video_driver_plugin_description(XineEngine::xine(), outputPlugins[i])));
                        break;
                    }
                }
            }
            break;
        case Phonon::VideoCaptureDeviceType:
            switch (index) {
                case 30000:
                    ret.insert("name", "USB Webcam");
                    ret.insert("description", "first description");
                    break;
                case 30001:
                    ret.insert("name", "DV");
                    ret.insert("description", "second description");
                    break;
            }
            break;
        case Phonon::VisualizationType:
            break;
        case Phonon::AudioCodecType:
            break;
        case Phonon::VideoCodecType:
            break;
        case Phonon::ContainerFormatType:
            break;
        case Phonon::EffectType:
            {
                const char *const *postPlugins = xine_list_post_plugins_typed(XineEngine::xine(), XINE_POST_TYPE_AUDIO_FILTER);
                for (int i = 0; postPlugins[i]; ++i) {
                    if (0x7F000000 + i == index) {
                        ret.insert("name", QLatin1String(postPlugins[i]));
                        ret.insert("description", QLatin1String(xine_get_post_plugin_description(XineEngine::xine(), postPlugins[i])));
                        break;
                    }
                }
                const char *const *postVPlugins = xine_list_post_plugins_typed(XineEngine::xine(), XINE_POST_TYPE_VIDEO_FILTER);
                for (int i = 0; postVPlugins[i]; ++i) {
                    if (0x7E000000 + i == index) {
                        ret.insert("name", QLatin1String(postPlugins[i]));
                        break;
                    }
                }
            }
            break;
    }
    return ret;
}

bool Backend::startConnectionChange(QSet<QObject *> nodes)
{
    Q_UNUSED(nodes);
    // there's nothing we can do but hope the connection changes won't take too long so that buffers
    // would underrun. But we should be pretty safe the way xine works by not doing anything here.
    return true;
}

bool Backend::connectNodes(QObject *_source, QObject *_sink)
{
    SourceNode *source = qobject_cast<SourceNode *>(_source);
    SinkNode *sink = qobject_cast<SinkNode *>(_sink);
    if (!source || !sink) {
        return false;
    }
    // what streams to connect - i.e. all both nodes support
    const MediaStreamTypes types = source->outputMediaStreamTypes() & sink->inputMediaStreamTypes();
    if (sink->source() != 0 || source->sinks().contains(sink)) {
        return false;
    }
    source->addSink(sink);
    sink->setSource(source);
    return true;
}

bool Backend::disconnectNodes(QObject *_source, QObject *_sink)
{
    SourceNode *source = qobject_cast<SourceNode *>(_source);
    SinkNode *sink = qobject_cast<SinkNode *>(_sink);
    if (!source || !sink) {
        return false;
    }
    const MediaStreamTypes types = source->outputMediaStreamTypes() & sink->inputMediaStreamTypes();
    if (!source->sinks().contains(sink) || sink->source() != source) {
        return false;
    }
    source->removeSink(sink);
    sink->unsetSource(source);
    return true;
}

bool Backend::endConnectionChange(QSet<QObject *> nodes)
{
    // Now that we know (by looking at the subgraph of nodes formed by the given nodes) what has to
    // be rewired we go over the nodes in order (from sink to source) and rewire them (all called
    // from the xine thread).
    QList<WireCall> wireCallsUnordered;
    QList<WireCall> wireCalls;
    foreach (QObject *q, nodes) {
        SourceNode *source = qobject_cast<SourceNode *>(q);
        if (source) {
            foreach (SinkNode *sink, source->sinks()) {
                WireCall w(source, sink);
                if (wireCallsUnordered.contains(w)) {
                    Q_ASSERT(!wireCalls.contains(w));
                    wireCalls << w;
                } else {
                    wireCallsUnordered << w;
                }
            }
        }
        SinkNode *sink = qobject_cast<SinkNode *>(q);
        if (sink && sink->source()) {
            WireCall w(sink->source(), sink);
            if (wireCallsUnordered.contains(w)) {
                Q_ASSERT(!wireCalls.contains(w));
                wireCalls << w;
            } else {
                wireCallsUnordered << w;
            }
        }
    }
    qSort(wireCalls);
    QCoreApplication::postEvent(XineEngine::thread(), new RewireEvent(wireCalls));
    return true;
}

void Backend::freeSoundcardDevices()
{
}

}}

#include "backend.moc"

// vim: sw=4 ts=4
