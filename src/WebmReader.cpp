/*
*      Copyright (C) 2016 - 2019 peak3d
*      http://www.peak3d.de
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  <http://www.gnu.org/licenses/>.
*
*/

#include "WebmReader.h"
#include "Ap4ByteStream.h"

#include <webm/reader.h>
#include <webm/webm_parser.h>

class WebmAP4Reader : public webm::Reader
{
public:
  WebmAP4Reader(AP4_ByteStream *stream) :m_stream(stream) {};

  webm::Status Run(webm::Callback *callback)
  {
    return m_parser.Feed(callback, this);
  }

  void Reset()
  {
    m_parser.DidSeek();
  }

  webm::Status Read(std::size_t num_to_read, std::uint8_t* buffer,
    std::uint64_t* num_actually_read) override
  {
    AP4_Size num_read;
    AP4_Result status = m_stream->ReadPartial(buffer, num_to_read, num_read);
    *num_actually_read = num_read;

    if (AP4_SUCCEEDED(status))
    {
      if (num_to_read == num_read)
        return webm::Status(webm::Status::kOkCompleted);
      else if (num_read)
        return webm::Status(webm::Status::kOkPartial);
    }
    return webm::Status(webm::Status::kEndOfFile);
  }

  webm::Status Skip(std::uint64_t num_to_skip,
    std::uint64_t* num_actually_skipped) override
  {
    AP4_Position pos;
    if (AP4_FAILED(m_stream->Tell(pos) || m_stream->Seek(pos + num_to_skip)))
      return webm::Status(webm::Status::kEndOfFile);

    *num_actually_skipped = num_to_skip;
    return webm::Status(webm::Status::kOkCompleted);
  }

  std::uint64_t Position() const override
  {
    AP4_Position pos(0);
    if (AP4_FAILED(m_stream->Tell(pos)))
      return ~0ULL;
    return pos;
  }

private:
  AP4_ByteStream *m_stream;
  webm::WebmParser m_parser;
};

/*************************************************************/

WebmReader::WebmReader(AP4_ByteStream *stream)
  : m_reader(new WebmAP4Reader(stream))
{
}

bool WebmReader::Initialize()
{
  return m_reader->Run(this).ok() && m_framePresent;
}

WebmReader::~WebmReader()
{
  delete m_reader, m_reader = nullptr;
}

void WebmReader::Reset(bool resetPackets)
{
  m_reader->Reset();
  m_framePresent = false;
}

bool WebmReader::StartStreaming(AP4_UI32 typeMask)
{
  return false;
}

bool WebmReader::GetInformation(INPUTSTREAM_INFO &info)
{
  return false;
}

// We assume that m_startpos is the current I-Frame position
bool WebmReader::SeekTime(uint64_t timeInTs, bool preceeding)
{
  return false;
}

bool WebmReader::ReadPacket(bool scanStreamInfo)
{
  if (m_framePresent)
  {
    // TODO:: Eat data

    m_framePresent = false;
    m_reader->Run(this);
    return true;
  }
  return false;
}

webm::Status WebmReader::OnFrame(const webm::FrameMetadata& metadata, webm::Reader* reader, std::uint64_t* bytes_remaining)
{
  m_framePresent = true;
  return webm::Status(webm::Status::kWouldBlock);
}