/**
 * @file vci_uds_transaction.cpp
 * @brief Implementation of the blocking UDS TP transaction class.
 *
 * Author: leiwei
 * Copyright (c) 2025. All rights reserved.
 */

#include "uds/vci_uds_transaction.h"
#include "uds/vci_uds_tp_utils.h"
#include <common_api/precision_timer.h>
#include <common_api/logger_macros.h>
#include <stdexcept>
#include <algorithm> // For std::min

namespace vci {
namespace uds {
namespace tp {

Transaction::Transaction(const UdsSessionContext& context, FrameSender sender, FrameProvider provider,
                         const std::vector<uint8_t>& payload)
    : m_context(context),
    m_sender(std::move(sender)),
    m_provider(std::move(provider)),
    m_request_payload(payload),
	// TODO g_timer = Common::PrecisionTimer::getInstance();
    m_timer(Common::PrecisionTimer::getInstance()),
    m_state(State::START),
    m_result({false, VCI_UDS_RESULT_INTERNAL_ERROR, {}})
{
    if (!m_sender || !m_provider || m_request_payload.empty()) {
        LOG_ERROR("[VCI-UDS-TP] Invalid arguments for Transaction constructor.");
        throw std::invalid_argument("Invalid arguments for Transaction constructor.");
    }
    LOG_INFO("[VCI-UDS-TP] New transaction created. ReqID: {:#x}, ResID: {:#x}, Payload size: {}.",
             m_context.request_id, m_context.response_id, m_request_payload.size());
}

void Transaction::stopExecution() {
    m_abort_flag.store(true, std::memory_order_release);
}

TransactionResult Transaction::execute() {
    setState(State::START);
    while (m_state != State::COMPLETED && m_state != State::FAILED) {
        if (m_abort_flag.load(std::memory_order_acquire)) {
            LOG_WARN("[VCI-UDS-TP] Transaction aborted by user request.");
            fail(VCI_UDS_RESULT_ABORTED);
            break;
        }
        run_state_machine();
    }

    if (m_result.success) {
        LOG_INFO("[VCI-UDS-TP] Transaction completed successfully. Response size: {}.", m_result.response_payload.size());
    } else {
        LOG_WARN("[VCI-UDS-TP] Transaction failed. Reason: {} ({})",
                 ProtocolUtils::getErrorString(m_result.result_code), m_result.result_code);
    }
    return m_result;
}

std::optional<FkVciCanDataType> Transaction::waitForFrame(uint64_t timeout_ms) {
    const uint64_t start_time = m_timer.getTickCount();
    while (true) {
        if (m_abort_flag.load(std::memory_order_acquire)) {
            return std::nullopt; // Abort signal received
        }

        const uint64_t elapsed_ms = m_timer.getTickCount() - start_time;
        if (elapsed_ms >= timeout_ms) {
            break; // Master timeout
        }

        const uint64_t wait_chunk_ms = 10;
        const uint64_t remaining_time_ms = timeout_ms - elapsed_ms;

        auto can_frame_opt = m_provider(std::min(wait_chunk_ms, remaining_time_ms > 0 ? remaining_time_ms : 1));
        if (can_frame_opt && can_frame_opt->CanID == m_context.response_id) {
            return can_frame_opt;
        }
    }
    return std::nullopt; // Timeout
}

void Transaction::setState(State new_state) {
    if (m_state != new_state) {
        m_state = new_state;
    }
}

void Transaction::fail(VciUdsResultCode error_code) {
    m_result.success = false;
    m_result.result_code = error_code;
    setState(State::FAILED);
}

void Transaction::run_state_machine() {
    switch (m_state) {
    case State::START:                         handle_start(); break;
    case State::SEND_SINGLE_FRAME:             handle_send_single_frame(); break;
    case State::SEND_FIRST_FRAME:              handle_send_first_frame(); break;
    case State::WAIT_FOR_FC:                   handle_wait_for_fc(); break;
    case State::SEND_CONSECUTIVE_FRAMES:       handle_send_consecutive_frames(); break;
    case State::WAIT_FOR_RESPONSE:             handle_wait_for_response(); break;
    case State::RECEIVE_CONSECUTIVE_FRAMES:    handle_receive_consecutive_frames(); break;
    default:
        LOG_ERROR("[VCI-UDS-TP] Reached invalid state: {}.", static_cast<int>(m_state));
        fail(VCI_UDS_RESULT_INTERNAL_ERROR);
        break;
    }
}

// --- Handler implementations ---

void Transaction::handle_start() {
    const size_t max_sf_payload = m_context.can_type != VCI_UDS_CAN_CLASSIC ? 62 : 7; // 62 for FD (SF_DL), 7 for Classic
    if (m_request_payload.size() <= max_sf_payload) {
        setState(State::SEND_SINGLE_FRAME);
    } else {
        const size_t max_payload = m_context.can_type != VCI_UDS_CAN_CLASSIC ? 0xFFFFFFFF : 4095;
        if (m_request_payload.size() > max_payload) {
            LOG_ERROR("[VCI-UDS-TP] Request payload ({} bytes) is too large.", m_request_payload.size());
            fail(VCI_UDS_RESULT_PAYLOAD_TOO_LARGE);
        } else {
            m_segmenter.emplace(m_request_payload.data(), m_request_payload.size(), m_context.can_type);
            setState(State::SEND_FIRST_FRAME);
        }
    }
}

void Transaction::handle_send_single_frame() {
    UdsTp_Frame tp_frame;
    tp_frame.pci_type = UDS_TP_PCI_TYPE_SINGLE_FRAME;
    tp_frame.frame.sf.payload_size = m_request_payload.size();
    tp_frame.frame.sf.payload = m_request_payload.data();

    LOG_DEBUG("[VCI-UDS-TP] Sending Single Frame ({} bytes).", tp_frame.frame.sf.payload_size);
    if (sendFrame(tp_frame)) {
        setState(State::WAIT_FOR_RESPONSE);
    } else {
        LOG_ERROR("[VCI-UDS-TP] Failed to send Single Frame.");
        fail(VCI_UDS_RESULT_SEND_FAILED);
    }
}

void Transaction::handle_send_first_frame() {
    if (!m_segmenter) {
        fail(VCI_UDS_RESULT_INTERNAL_ERROR);
        return;
    }

    LOG_DEBUG("[VCI-UDS-TP] Sending First Frame (Total size: {}).", m_segmenter->getTotalSize());
    UdsTp_Frame tp_frame = m_segmenter->getNextFrame();
    if (sendFrame(tp_frame)) {
        setState(State::WAIT_FOR_FC);
    } else {
        LOG_ERROR("[VCI-UDS-TP] Failed to send First Frame.");
        fail(VCI_UDS_RESULT_SEND_FAILED);
    }
}

void Transaction::handle_wait_for_fc() {
    auto can_frame_opt = waitForFrame(m_context.tp_config.n_bs_timeout);
    if (!can_frame_opt) {
        if (m_abort_flag.load(std::memory_order_acquire)) {
            fail(VCI_UDS_RESULT_ABORTED);
        } else {
            LOG_WARN("[VCI-UDS-TP] Timeout waiting for Flow Control.");
            fail(VCI_UDS_RESULT_TIMEOUT_BS);
        }
        return;
    }

    UdsTp_Frame tp_frame;
    if (!ProtocolUtils::parse(*can_frame_opt, tp_frame) || tp_frame.pci_type != UDS_TP_PCI_TYPE_FLOW_CONTROL) {
        LOG_WARN("[VCI-UDS-TP] Expected Flow Control, but received an invalid or different frame.");
        fail(VCI_UDS_RESULT_UNEXPECTED_FRAME);
        return;
    }

    const auto& fc = tp_frame.frame.fc;
    LOG_DEBUG("[VCI-UDS-TP] Flow Control received. Status: {}, BS: {}, STmin: {}.", static_cast<int>(fc.status), fc.block_size, fc.separation_time);

    switch (fc.status) {
    case UDS_TP_FLOW_STATUS_CONTINUE_TO_SEND:
        m_fc_block_size_from_ecu = fc.block_size;
        m_fc_frames_sent_in_block = 0;
        if (fc.separation_time <= 0x7F) {
            m_fc_separation_time_from_ecu = fc.separation_time;
        } else if (fc.separation_time >= 0xF1 && fc.separation_time <= 0xF9) {
            m_fc_separation_time_from_ecu = 1;  // Treat sub-millisecond delays as 1ms for platform compatibility
        } else {
            m_fc_separation_time_from_ecu = 127; // Invalid values, default to max
        }
        setState(State::SEND_CONSECUTIVE_FRAMES);
        break;
    case UDS_TP_FLOW_STATUS_WAIT:
        break;
    case UDS_TP_FLOW_STATUS_OVERFLOW:
        fail(VCI_UDS_RESULT_FC_OVERFLOW);
        break;
    }
}

void Transaction::handle_send_consecutive_frames() {
    if (!m_segmenter) {
        fail(VCI_UDS_RESULT_INTERNAL_ERROR);
        return;
    }

    while (!m_segmenter->isDone()) {
        if (m_abort_flag.load(std::memory_order_acquire)) {
            fail(VCI_UDS_RESULT_ABORTED);
            return;
        }

        if (m_fc_block_size_from_ecu > 0 && m_fc_frames_sent_in_block >= m_fc_block_size_from_ecu) {
            m_fc_frames_sent_in_block = 0;
            setState(State::WAIT_FOR_FC);
            return;
        }

        if (m_fc_separation_time_from_ecu > 0) {
            uint64_t wait_start = m_timer.getTickCount();
            while (m_timer.getTickCount() - wait_start < m_fc_separation_time_from_ecu) {
                if (m_abort_flag.load(std::memory_order_acquire)) {
                    fail(VCI_UDS_RESULT_ABORTED);
                    return;
                }
                m_timer.waitFor(1);
            }
        }

        UdsTp_Frame tp_frame = m_segmenter->getNextFrame();
        if (!sendFrame(tp_frame)) {
            LOG_ERROR("[VCI-UDS-TP] Failed to send Consecutive Frame (SN: {}).", tp_frame.frame.cf.sequence_number);
            fail(VCI_UDS_RESULT_SEND_FAILED);
            return;
        }
        m_fc_frames_sent_in_block++;
    }

    LOG_DEBUG("[VCI-UDS-TP] All Consecutive Frames sent.");
    setState(State::WAIT_FOR_RESPONSE);
}

void Transaction::handle_wait_for_response() {
    auto timeout = m_context.tp_config.n_as_timeout;

    while (true) {
        auto can_frame_opt = waitForFrame(timeout);
        if (!can_frame_opt) {
            if (m_abort_flag.load(std::memory_order_acquire)) {
                fail(VCI_UDS_RESULT_ABORTED);
            } else {
                LOG_WARN("[VCI-UDS-TP] Timeout waiting for response (timeout: {}ms).", timeout);
                fail((m_nrc78_count > 0) ? VCI_UDS_RESULT_TIMEOUT_P2_STAR : VCI_UDS_RESULT_TIMEOUT_A);
            }
            return;
        }

        UdsTp_Frame tp_frame;
        if (!ProtocolUtils::parse(*can_frame_opt, tp_frame)) {
            LOG_WARN("[VCI-UDS-TP] Failed to parse response frame, continuing to wait.");
            continue;
        }

        if (tp_frame.pci_type == UDS_TP_PCI_TYPE_SINGLE_FRAME) {
            const auto& sf = tp_frame.frame.sf;
            std::vector<uint8_t> temp_payload(sf.payload, sf.payload + sf.payload_size);
            LOG_INFO("[VCI-UDS-TP] Received Single Frame response ({} bytes).", temp_payload.size());

            if (temp_payload.size() == 3 && temp_payload[0] == 0x7F && temp_payload[2] == 0x78) {
                m_nrc78_count++;
                LOG_INFO("[VCI-UDS-TP] Received NRC 0x78 (Response Pending). Count: {}. Extending timeout.", m_nrc78_count);

                if (m_nrc78_count >= m_context.tp_config.max_nrc78_count) {
                    LOG_ERROR("[VCI-UDS-TP] Exceeded maximum number of NRC 78 responses (limit: {}).", m_context.tp_config.max_nrc78_count);
                    fail(VCI_UDS_RESULT_NRC78_LIMIT_EXCEEDED);
                    return;
                }
                timeout = m_context.tp_config.n_ar_timeout;
                continue;
            }

            m_result.response_payload = std::move(temp_payload);
            if (!m_result.response_payload.empty() && m_result.response_payload[0] == 0x7F) {
                fail(VCI_UDS_RESULT_NEGATIVE_RESPONSE);
            } else {
                m_result.success = true;
                m_result.result_code = VCI_UDS_RESULT_OK;
                setState(State::COMPLETED);
            }
            return;

        } else if (tp_frame.pci_type == UDS_TP_PCI_TYPE_FIRST_FRAME) {
            LOG_INFO("[VCI-UDS-TP] Received First Frame response, starting multi-frame reception.");
            auto status = m_reassembler.processFrame(tp_frame);

            if (status == Reassembler::Status::IN_PROGRESS) {
                UdsTp_Frame fc_frame;
                fc_frame.pci_type = UDS_TP_PCI_TYPE_FLOW_CONTROL;
                fc_frame.frame.fc = {UDS_TP_FLOW_STATUS_CONTINUE_TO_SEND, m_context.tp_config.block_size, m_context.tp_config.st_min};

                if (sendFrame(fc_frame)) {
                    setState(State::RECEIVE_CONSECUTIVE_FRAMES);
                } else {
                    fail(VCI_UDS_RESULT_SEND_FAILED);
                }
            } else if (status == Reassembler::Status::COMPLETE) {
                m_result.response_payload = m_reassembler.getPayload();
                m_result.success = true;
                m_result.result_code = VCI_UDS_RESULT_OK;
                setState(State::COMPLETED);
            } else {
                fail(VCI_UDS_RESULT_UNEXPECTED_FRAME);
            }
            return;

        } else {
            LOG_WARN("[VCI-UDS-TP] Expected SF or FF, but received frame type {}. Continuing to wait.", static_cast<int>(tp_frame.pci_type));
            continue;
        }
    }
}

void Transaction::handle_receive_consecutive_frames() {
    while (m_reassembler.getStatus() == Reassembler::Status::IN_PROGRESS) {
        auto can_frame_opt = waitForFrame(m_context.tp_config.n_cr_timeout);
        if (!can_frame_opt) {
            if (m_abort_flag.load(std::memory_order_acquire)) {
                fail(VCI_UDS_RESULT_ABORTED);
            } else {
                LOG_WARN("[VCI-UDS-TP] Timeout waiting for Consecutive Frame.");
                fail(VCI_UDS_RESULT_TIMEOUT_CR);
            }
            return;
        }

        UdsTp_Frame tp_frame;
        if (!ProtocolUtils::parse(*can_frame_opt, tp_frame)) {
            continue;
        }

        auto status = m_reassembler.processFrame(tp_frame);
        if (status == Reassembler::Status::ERROR_SEQUENCE) {
            fail(VCI_UDS_RESULT_SEQUENCE_ERROR);
            return;
        } else if (status == Reassembler::Status::ERROR_UNEXPECTED_FRAME) {
            fail(VCI_UDS_RESULT_UNEXPECTED_FRAME);
            return;
        }
    }

    if (m_reassembler.getStatus() == Reassembler::Status::COMPLETE) {
        m_result.response_payload = m_reassembler.getPayload();
        LOG_INFO("[VCI-UDS-TP] Multi-frame response reception complete ({} bytes).", m_result.response_payload.size());

        if (!m_result.response_payload.empty() && m_result.response_payload[0] == 0x7F) {
            fail(VCI_UDS_RESULT_NEGATIVE_RESPONSE);
        } else {
            m_result.success = true;
            m_result.result_code = VCI_UDS_RESULT_OK;
            setState(State::COMPLETED);
        }
    } else {
        fail(VCI_UDS_RESULT_INTERNAL_ERROR);
    }
}

bool Transaction::sendFrame(const UdsTp_Frame& tp_frame) {
    FkVciCanDataType can_frame = ProtocolUtils::build(tp_frame, m_context.request_id, m_context.can_type, m_context.padding_target_size, m_context.padding_fill_byte);
    return m_sender(can_frame);
}

} // namespace tp
} // namespace uds
} // namespace vci
