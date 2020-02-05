/**
 * @file CUdpMultiCastReciver.cpp
 * @author DennisMi (https://www.dennisthink.com/)
 * @brief UDP���շ����ݵ���ʵ���ļ�
 * @version 0.1
 * @date 2019-12-27
 *
 * @copyright Copyright (c) 2019
 *
 */

#include "CUdpMultiCastReciver.h"
namespace ClientCore {
	std::shared_ptr<spdlog::logger> CUdpMultiCastReciver::ms_loger;

	CUdpMultiCastReciver::CUdpMultiCastReciver(asio::io_context& ioService, const std::string strIp, const int port, RECIVER_CALL_BACK&& callBack) :m_callBack(callBack)
	{
		m_udpServerIp = strIp;
		m_udpServerPort = port;
		asio::ip::address listenAddress = asio::ip::make_address(strIp);
		asio::ip::udp::endpoint listenPt(listenAddress, port);
		m_udpSocket = std::make_shared<asio::ip::udp::socket>(ioService);
		
		m_udpSocket->open(listenPt.protocol());
		m_udpSocket->set_option(asio::ip::udp::socket::reuse_address(true));
		m_udpSocket->bind(listenPt);

		asio::ip::address multiCastAddress = asio::ip::make_address(strIp);
		std::error_code ec;

		m_udpSocket->set_option(asio::ip::multicast::join_group(multiCastAddress),ec);
		LOG_INFO(ms_loger, "EC:{} {} [{} {}]", ec.value(), ec.message(), __FILENAME__, __LINE__);
	}

	void CUdpMultiCastReciver::StartConnect()
	{
		do_read();
	}

	/**
	 * @brief ��UDP��ȡ����
	 *
	 */
	void CUdpMultiCastReciver::do_read()
	{
		if (m_udpSocket)
		{
			auto pSelf = shared_from_this();
			m_udpSocket->async_receive_from(asio::buffer(m_recvbuf, max_msg_length_udp), m_recvFromPt, [this, pSelf](std::error_code ec, std::size_t bytes) {
				if (!ec && bytes > 0)
				{
					TransBaseMsg_t trans(m_recvbuf);
					if (bytes >= 8 && bytes > trans.GetSize())
					{
						if (trans.GetType() != E_MsgType::FileSendDataReq_Type && trans.GetType() != E_MsgType::FileRecvDataReq_Type)
						{
							LOG_INFO(this->ms_loger, "[{}] UDP RECV: {} MSG:{} {} [{} {}]", UserId(), EndPoint(m_recvFromPt), MsgType(trans.GetType()), trans.to_string(), __FILENAME__, __LINE__);
						}
						handle_msg(m_recvFromPt, &trans);
					}
					do_read();
				}
			});
		}
	}

	/**
	 * @brief �����յ���UDP��Ϣ
	 *
	 * @param endPt UDP��Ϣ�ķ����ߵĵ�ַ
	 * @param pMsg UDP��Ϣ
	 */
	void CUdpMultiCastReciver::handle_msg(const asio::ip::udp::endpoint endPt, TransBaseMsg_t* pMsg)
	{
		m_callBack(endPt, pMsg);
	}


	/**
	 * @brief ������Ϣ��UDP������
	 *
	 * @param pMsg
	 */
	void CUdpMultiCastReciver::sendToServer(const BaseMsg* pMsg)
	{
		send_msg(m_udpServerIp, m_udpServerPort, pMsg);
	}


	/**
	 * @brief ������Ϣ����Ӧ��IP�Ͷ˿�
	 *
	 * @param strIp UDP��IP
	 * @param port UDP�Ķ˿�
	 * @param pMsg �����͵���Ϣ
	 */
	void CUdpMultiCastReciver::send_msg(const std::string strIp, const int port, const BaseMsg* pMsg)
	{
		asio::ip::udp::resolver resolver(m_udpSocket->get_io_context());
		asio::ip::udp::resolver::results_type endpoints =
			resolver.resolve(asio::ip::udp::v4(), strIp, std::to_string(port));

		if (!endpoints.empty())
		{

			auto pSend = std::make_shared<TransBaseMsg_t>(pMsg->GetMsgType(), pMsg->ToString());
			send_msg(*endpoints.begin(), pSend);
		}
	}

	/**
	 * @brief ��ȡUDP��ַ���ַ�����ʾ
	 *
	 * @param endPt UDP��ַ
	 * @return std::string ��ַ���ַ�����ʾ
	 */
	std::string CUdpMultiCastReciver::EndPoint(const asio::ip::udp::endpoint endPt)
	{
		std::string strResult = endPt.address().to_string() + ":" + std::to_string(endPt.port());
		return strResult;
	}

	/**
	 * @brief ����UDP��Ϣ
	 *
	 * @param endPt
	 * @param pMsg
	 */
	void CUdpMultiCastReciver::send_msg(const asio::ip::udp::endpoint endPt, const BaseMsg* pMsg)
	{
		auto pSend = std::make_shared<TransBaseMsg_t>(pMsg->GetMsgType(), pMsg->ToString());
		send_msg(endPt, pSend);
	}

	/**
	 * @brief ����UDP��Ϣ
	 *
	 * @param endPt
	 * @param pMsg
	 */
	void CUdpMultiCastReciver::send_msg(const asio::ip::udp::endpoint endPt, TransBaseMsg_S_PTR pMsg)
	{
		if (pMsg->GetType() != E_MsgType::FileSendDataReq_Type && pMsg->GetType() != E_MsgType::FileRecvDataReq_Type)
		{
			LOG_INFO(ms_loger, "[{}] UDP SEND: {} Msg:{} {} [{} {}]", UserId(), EndPoint(endPt), MsgType(pMsg->GetType()), pMsg->to_string(), __FILENAME__, __LINE__);
		}
		//m_sendQueue.push({ endPt,pMsg });
		/*if (!m_bDoSend) {
			m_bDoSend = true;
			do_SendMsg();
		}*/

		if (m_udpSocket)
		{
			memcpy(m_sendbuf, pMsg->GetData(), pMsg->GetSize());
			try {
				m_udpSocket->async_send_to(asio::buffer(m_sendbuf, pMsg->GetSize()), endPt, [this](std::error_code /*ec*/, std::size_t /*bytes*/) {
				});
			}
			catch (std::exception ec)
			{
				LOG_INFO(ms_loger, "{}", ec.what());
			}
		}
	}
	void CUdpMultiCastReciver::SendMultiCastMsg(const UdpMultiCastReqMsg& reqMsg)
	{

	}
	void CUdpMultiCastReciver::DoSend() {
		//if (!m_bDoSend) {
		//	m_bDoSend = true;
		//	do_SendMsg();
		//}
	}
	void CUdpMultiCastReciver::do_SendMsg() {
		/*if (!m_sendQueue.empty()) {
			if (m_udpSocket)
			{
				auto item = m_sendQueue.front();
				memcpy(m_sendbuf, item.msgToSend->GetData(), item.msgToSend->GetSize());
				try {
					auto pSelf = shared_from_this();
					m_udpSocket->async_send_to(asio::buffer(m_sendbuf, item.msgToSend->GetSize()), item.endPt, [this, pSelf](std::error_code ec, std::size_t bytes) {
						if (!ec && bytes > 0) {
							m_sendQueue.pop();
							do_SendMsg();
						}
					});
				}
				catch (std::exception ec)
				{
					LOG_INFO(ms_loger, "{}", ec.what());
				}
			}
		}
		else
		{
			m_bDoSend = false;
		}*/
	}

}