/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>

#include <dds/DCPS/StaticIncludes.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "ControlProtocolTypeSupportImpl.h"


#include <future>
#include <iostream>
#include <string>

#include "zmq.hpp"
#include "zmq_addon.hpp"
#include "json.hpp"

#define DDS_FLAG 1

void ZMQ_PublisherThread(zmq::context_t* ctx, DDS::DataWriter_var& msg_writer) {

}

void ZMQ_SubscriberThread(zmq::context_t* ctx, DDS::DataWriter_var& writer) {
    DDS_AGENT::ControlMessageDataWriter_var message_writer =
        DDS_AGENT::ControlMessageDataWriter::_narrow(writer);

    // construct a REP (reply) socket and bind to interface
    zmq::socket_t socket{ *ctx, zmq::socket_type::sub };
    socket.connect("tcp://localhost:5555");
    socket.set(zmq::sockopt::subscribe, "");

    //socket.bind("tcp://*:5555");

    std::cout << "zmq ready" << std::endl;
    
    socket.connect("tcp://localhost:5555");
    socket.set(zmq::sockopt::subscribe, "");


    for (;;)
    {
        zmq::message_t json;
        // receive a request from client
        socket.recv(json, zmq::recv_flags::none);
        auto reply_str = std::string(static_cast<char*>(json.data()), json.size());


        zmq::message_t request;
        // receive a request from client
        socket.recv(request, zmq::recv_flags::none);

        // Write samples
        DDS_AGENT::ControlMessage message;
        message.obj_id = 19;
        message.obj_data = 612;
        message.json = reply_str.c_str();
        message.img.length(request.size());
        memcpy(message.img.get_buffer(), request.data(), request.size());

        DDS::ReturnCode_t error = message_writer->write(message, DDS::HANDLE_NIL);

        if (error != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR,
                ACE_TEXT("ERROR: %N:%l: main() -")
                ACE_TEXT(" write returned %d!\n"), error));
        }


    }
}

void DDS_SvcThread(zmq::context_t* ctx, DDS::DataWriter_var& writer) {
    
    DDS_AGENT::ControlMessageDataWriter_var message_writer =
        DDS_AGENT::ControlMessageDataWriter::_narrow(writer);

    // Block until Subscriber is available
    DDS::StatusCondition_var condition = writer->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("Block until subscriber is available\n")));

    while (true) {
        DDS::PublicationMatchedStatus matches;
        if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
          
        }

        if (matches.current_count >= 1) {
            break;
        }

        DDS::ConditionSeq conditions;
        DDS::Duration_t timeout = { 600, 0 };
        if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
            
        }
    }

    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("Subscriber is available\n")));

    ws->detach_condition(condition);

    // Wait for samples to be acknowledged
    DDS::Duration_t timeout = { 30, 0 };
    if (message_writer->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK) {

    }
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
#if DDS_FLAG
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
        dpf->create_participant(42,
            PARTICIPANT_QOS_DEFAULT,
            0,
            OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!participant) {
        ACE_ERROR_RETURN((LM_ERROR,
            ACE_TEXT("ERROR: %N:%l: main() -")
            ACE_TEXT(" create_participant failed!\n")),
            1);
    }
    std::cout << "participant" << std::endl;

    // Register TypeSupport (Messenger::Message)
    DDS_AGENT::ControlMessageTypeSupport_var ts =
        new DDS_AGENT::ControlMessageTypeSupportImpl;

    if (ts->register_type(participant, "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
            ACE_TEXT("ERROR: %N:%l: main() -")
            ACE_TEXT(" register_type failed!\n")),
            1);
    }
    std::cout << "register_type" << std::endl;

    // Create Topic (Movie Discussion List)
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
            type_name,
            TOPIC_QOS_DEFAULT,
            0,
            OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!topic) {
        ACE_ERROR_RETURN((LM_ERROR,
            ACE_TEXT("ERROR: %N:%l: main() -")
            ACE_TEXT(" create_topic failed!\n")),
            1);
    }
    std::cout << "topic" << std::endl;
    // Create Publisher
    DDS::Publisher_var publisher =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
            0,
            OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!publisher) {
        ACE_ERROR_RETURN((LM_ERROR,
            ACE_TEXT("ERROR: %N:%l: main() -")
            ACE_TEXT(" create_publisher failed!\n")),
            1);
    }

    // Create DataWriter
    DDS::DataWriter_var writer =
        publisher->create_datawriter(topic,
            DATAWRITER_QOS_DEFAULT,
            0,
            OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!writer) {
        ACE_ERROR_RETURN((LM_ERROR,
            ACE_TEXT("ERROR: %N:%l: main() -")
            ACE_TEXT(" create_datawriter failed!\n")),
            1);
    } 

    std::cout << "create_datawriter" << std::endl;

    zmq::context_t ctx(1);
    auto thread2 = std::async(std::launch::async, ZMQ_SubscriberThread, &ctx, writer);

    auto thread3 = std::async(std::launch::async, DDS_SvcThread, &ctx, writer);

    //auto thread1 = std::async(std::launch::async, ZMQ_PublisherThread, &ctx);

    // Give the publisher a chance to bind, since inproc requires it
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    thread3.wait();
    //thread1.wait();
    thread2.wait();

  try {
   

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant);

    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return 1;
  }
#endif

  return 0;
}
