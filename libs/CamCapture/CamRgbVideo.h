#pragma once
//============================================================================
// Copyright (C) 2025 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <memory>

class CamRgbVideo
{
public:
    CamRgbVideo(std::shared_ptr<uint8_t>& vidData,
                int         vidDataLen,
				int			width,
                int			height )
        : m_VidData( vidData )
		, m_VidDataLen( vidDataLen )
		, m_Width( width )
		, m_Height( height )
	{
	}

	std::shared_ptr<uint8_t>	m_VidData;
    int                         m_VidDataLen;
	int							m_Width;
	int							m_Height; 
};
