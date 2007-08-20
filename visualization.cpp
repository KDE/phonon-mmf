/*  This file is part of the KDE project
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>

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

#include "visualization.h"
#include <kdebug.h>

namespace Phonon
{
namespace Xine
{

class VisualizationXT : public SinkNodeXT, public SourceNodeXT
{
    public:
        void rewireTo(SourceNodeXT *);
        xine_post_out_t *videoOutputPort() const;
};

#define K_XT(type) (static_cast<type *>(SinkNode::threadSafeObject.data()))
Visualization::Visualization( QObject* parent )
    : QObject(parent),
    SinkNode(new VisualizationXT),
    SourceNode(K_XT(VisualizationXT))
{
}

int Visualization::visualization() const
{
	return m_visualization;
}

void Visualization::setVisualization( int newVisualization )
{
	m_visualization = newVisualization;
}

void VisualizationXT::rewireTo(SourceNodeXT *)
{
    kFatal() << "not implemented" << endl;
}

xine_post_out_t *VisualizationXT::videoOutputPort() const
{
    kFatal() << "not implemented" << endl;
    return 0;
}

}} //namespace Phonon::Xine

#include "visualization.moc"
// vim: sw=4 ts=4
