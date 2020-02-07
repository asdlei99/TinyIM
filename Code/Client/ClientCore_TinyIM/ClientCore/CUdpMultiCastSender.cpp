/**
 * @file CUdpMultiCastSender.cpp
 * @author DennisMi (https://www.dennisthink.com/)
 * @brief UDP���շ����ݵ���ʵ���ļ�
 * @version 0.1
 * @date 2019-12-27
 *
 * @copyright Copyright (c) 2019
 *
 */

#include "CUdpMultiCastSender.h"
namespace ClientCore {
	std::shared_ptr<spdlog::logger> CUdpMultiCastSender::ms_loger;

	CUdpMultiCastSender::CUdpMultiCastSender(asio::io_context& ioService, const std::string strIp, const int port, SENDER_CALL_BACK&& callBack) :m_callBack(callBack)
	{
		m_udpServerIp = strIp;
		m_udpServerPort = port;
		m_udpSocket = std::make_shared <asio::ip::udp::socket>(ioService);
		asio::ip::address toAddress = asio::ip::make_address(strIp);
		asio::ip::udp::endpoint toEndPt(toAddress, port);
		m_udpSocket->open(toEndPt.protocol());
		m_udpSocket->set_option(asio::ip::udp::socket::broadcast(true));
		m_udpServerPt = toEndPt;
		m_bRecvMsg = false;
		m_bDoRead = false;
	}

	void CUdpMultiCastSender::StartConnect()
	{
		//do_read();
	}

	/**
	 * @brief ��UDP��ȡ����
	 *
	 */
	void CUdpMultiCastSender::do_read()
	{
		if (!m_bDoRead)
		{
			if (m_udpSocket)
			{
				auto pSelf = shared_from_this();
				m_udpSocket->async_receive_from(asio::buffer(m_recvbuf, max_msg_length_udp), m_recvFromPt, [this, pSelf](std::error_code ec, std::size_t bytes) {
					if (!ec && bytes > 0)
					{
						TransBaseMsg_t trans(m_recvbuf);
						if (bytes >= 8 && bytes >= trans.GetSize())
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
			m_bDoRead = true;
		}
	}

	/**
	 * @brief �����յ���UDP��Ϣ
	 *
	 * @param endPt UDP��Ϣ�ķ����ߵĵ�ַ
	 * @param pMsg UDP��Ϣ
	 */
	void CUdpMultiCastSender::handle_msg(const asio::ip::udp::endpoint endPt, TransBaseMsg_t* pMsg)
	{
		m_callBack(endPt, pMsg);
		m_bRecvMsg = true;
	}


	/**
	 * @brief ������Ϣ��UDP������
	 *
	 * @param pMsg
	 */
	void CUdpMultiCastSender::sendToServer(const BaseMsg* pMsg)
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
	void CUdpMultiCastSender::send_msg(const std::string strIp, const int port, const BaseMsg* pMsg)
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
	std::string CUdpMultiCastSender::EndPoint(const asio::ip::udp::endpoint endPt)
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
	void CUdpMultiCastSender::send_msg(const asio::ip::udp::endpoint endPt, const BaseMsg* pMsg)
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
	void CUdpMultiCastSender::send_msg(const asio::ip::udp::endpoint endPt, TransBaseMsg_S_PTR pMsg)
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
			do_read();
		}
	}

	void CUdpMultiCastSender::SendMultiCastMsg(const UdpMultiCastReqMsg& reqMsg)
	{
		//if (m_udpSocket)
		//{
		//	memcpy(m_sendbuf, pMsg->GetData(), pMsg->GetSize());
		//	try {
		//		m_udpSocket->async_send_to(asio::buffer(m_sendbuf, pMsg->GetSize()), endPt, [this](std::error_code /*ec*/, std::size_t /*bytes*/) {
		//		});
		//	}
		//	catch (std::exception ec)
		//	{
		//		LOG_INFO(ms_loger, "{}", ec.what());
		//	}
		//}
	}
	void CUdpMultiCastSender::DoSend() {
		//if (!m_bDoSend) {
		//	m_bDoSend = true;
		//	do_SendMsg();
		//}
	}
	void CUdpMultiCastSender::do_SendMsg() {
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