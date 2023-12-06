/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include "DataReaderListenerImpl.h"
#include "ControlProtocolTypeSupportC.h"
#include "ControlProtocolTypeSupportImpl.h"

#include <iostream>
#include "json.hpp"

DataReaderListenerImpl::DataReaderListenerImpl()
{
    m_ctx = new zmq::context_t(2);
    m_socket = new zmq::socket_t( *m_ctx, zmq::socket_type::pub );
    m_socket->bind("tcp://*:5556");

    std::cout << "zmq ready" << std::endl;
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
    delete m_ctx;
    delete m_socket;
}

void
DataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedDeadlineMissedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedIncompatibleQosStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleRejectedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::LivelinessChangedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  DDS_AGENT::ControlMessageDataReader_var reader_i =
      DDS_AGENT::ControlMessageDataReader::_narrow(reader);

  if (!reader_i) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(1);
  }

  DDS_AGENT::ControlMessage message;
  DDS::SampleInfo info;

  const DDS::ReturnCode_t error = reader_i->take_next_sample(message, info);

  if (error == DDS::RETCODE_OK) {
    std::cout << "SampleInfo.sample_rank = " << info.sample_rank << std::endl;
    std::cout << "SampleInfo.instance_state = " << OpenDDS::DCPS::InstanceState::instance_state_mask_string(info.instance_state) << std::endl;

    if (info.valid_data) {
      std::cout << "Message: obj_id    = " << message.obj_id << std::endl
                << "         obj_data  = " << message.obj_data   << std::endl;


      //auto reply_json = nlohmann::json::parse(message.json.in());
      //std::cout << reply_json["dtype"] << " " << reply_json["shape"] << std::endl;
      int _size = strlen(message.json.in());
      zmq::message_t send_json(_size);
      memcpy(send_json.data(), message.json.in(), _size);
      m_socket->send(send_json);

      zmq::message_t send_img(message.img.maximum());
      //char* data = reinterpret_cast<char*>(request.data());
      //std::cout << request.size() << std::endl;
      memcpy(send_img.data(), message.img.get_buffer(), message.img.maximum());
      m_socket->send(send_img);
    }

  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" take_next_sample failed!\n")));
  }
}

void
DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SubscriptionMatchedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleLostStatus& /*status*/)
{
}
