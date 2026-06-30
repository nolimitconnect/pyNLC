#pragma once
//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <vector>

#include "AppCommon.h"
#include "GuiAudioMgr.h"
#include <libaudio-nlc/miniaudio/AudioFrameAecBuffer.h>

class AudioOutWaveformWidget : public QFrame {
    Q_OBJECT
public:
    explicit AudioOutWaveformWidget(QWidget *parent = nullptr);

    std::vector<AudioFrameAec>  getAudioSpeakerHistory( void );

protected:
    void                        showEvent(QShowEvent *event) override;

    void                        hideEvent(QHideEvent *event) override;

    void                        paintEvent(QPaintEvent *event) override;

    void                        drawWaveform(QPainter &p, const std::vector<AudioFrameAec> &frames, int yOffset, int h, QColor color, QString label);

    //=== vars ===//
    QTimer*                     m_refreshTimer;
};



