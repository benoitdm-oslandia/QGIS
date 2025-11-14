/***************************************************************************
    qgs3dprimitiveeditiontoolbar.cpp
    -------------------
    begin                : November 2025
    copyright            : (C) 2025 Oslandia
    email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgs3dprimitiveeditiontoolbar.h"
//#include "moc_qgs3dprimitiveeditiontoolbar.cpp"

#include <QAction>
#include <QLabel>

Qgs3DPrimitiveEditionToolBar::Qgs3DPrimitiveEditionToolBar( Qgs3DMapCanvasWidget *parent )
  : Qgs3DEditionToolBar( QStringLiteral( "Primitive edition" ), parent )
{
  addWidget( new QLabel( tr( "PRIMITIVE" ) ) );
}

void Qgs3DPrimitiveEditionToolBar::activate( QgsMapLayer * /*layer*/ )
{
  for ( auto action : findChildren<QAction *>() )
    action->setVisible( true );

  setEnabled( true );
}

void Qgs3DPrimitiveEditionToolBar::deactivate()
{
  for ( auto action : findChildren<QAction *>() )
    action->setVisible( false );

  setEnabled( false );
}

QList<QAction *> Qgs3DPrimitiveEditionToolBar::groupActions() const
{
  return {};
}
