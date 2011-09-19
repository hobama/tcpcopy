#include "../communication/msg.h"
#include "../event/select_server.h"
#include "nl_firewall.h"
#include "interception.h"
#include "status.h"
#include "delay.h"

static  int firewall_sock;
static  int msg_listen_sock;

static void set_sock_no_delay(int sock){
	int flag = 1;
	if(setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(char *)&flag,sizeof(flag)) == -1){
		perror("setsockopt:");
		logInfo(LOG_ERR,"setsockopt error");
		exit(errno);
	}
}

static void formatOutput(int level,struct iphdr *ip_header)
{
	if(output_level >= level)
	{
		return;
	}
	struct in_addr srcaddr;
	struct in_addr destaddr;
	srcaddr.s_addr=ip_header->saddr;
	destaddr.s_addr=ip_header->daddr;
	char* tmpbuf=inet_ntoa(srcaddr);
	char sbuf[1024];
	memset(sbuf,0,1024);
	strcpy(sbuf,tmpbuf);
	char dbuf[1024];
	memset(dbuf,0,1024);
	tmpbuf=inet_ntoa(destaddr);
	strcpy(dbuf,tmpbuf);
	size_t size_ip = ip_header->ihl<<2;
	struct tcphdr *tcp_header= (struct tcphdr*)((char *)ip_header+size_ip);
	uint32_t packSize=ntohs(ip_header->tot_len);
	unsigned int seq=ntohl(tcp_header->seq);
	unsigned int ack_seq=ntohl(tcp_header->ack_seq);
	{
		logInfo(level,"%s:%u-->%s:%u,length %u,seq=%u,ack_seq=%u",sbuf,
				ntohs(tcp_header->source),dbuf,ntohs(tcp_header->dest),
				packSize,seq,ack_seq);
	}

}

static int seq =0;
static unsigned char drop_buffer[128];
static int drop_netlink_packet(int packet_id)
{
	struct nlmsghdr* nl_header=(struct nlmsghdr*)drop_buffer;
	nl_header->nlmsg_type=IPQM_VERDICT;
	nl_header->nlmsg_len=NLMSG_LENGTH(sizeof(struct ipq_verdict_msg));
	nl_header->nlmsg_flags=(NLM_F_REQUEST);
	nl_header->nlmsg_pid=getpid();
	nl_header->nlmsg_seq=seq++;
	struct ipq_verdict_msg *ver_data = NULL;
	ver_data=(struct ipq_verdict_msg *)NLMSG_DATA(nl_header);
	ver_data->value=NF_DROP;
	ver_data->id=packet_id;
	struct sockaddr_nl addr;
	memset(&addr,0,sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = 0;
	addr.nl_groups = 0;
	if(sendto(firewall_sock,(void *)nl_header,nl_header->nlmsg_len,0,
				(struct sockaddr *)&addr,sizeof(struct sockaddr_nl))<0)
	{
		perror("unable to send mode message");
		logInfo(LOG_ERR,"unable to send mode message");
		exit(0);
	}
	return 1;
}

static int recvFromTest=0;
static void interception_process(int fd){
	if(fd == msg_listen_sock){
		int newfd = accept(msg_listen_sock,NULL,NULL);	
		set_sock_no_delay(newfd);
		if(newfd != -1){
			select_sever_add(newfd);
		}
	}else if(fd == firewall_sock){
		int packet_id=0;
		struct iphdr *ip_header = nl_firewall_recv(firewall_sock,&packet_id);
		status_update(ip_header);
		recvFromTest++;
		{
			formatOutput(LOG_DEBUG,ip_header);
		}
		//drop the packet
		drop_netlink_packet(packet_id);  	
	}else{
		struct copyer_msg_st *c_msg = msg_receiver_recv(fd);
		if(c_msg){
			if(c_msg->type == CLIENT_ADD){
				status_add(c_msg->client_ip,c_msg->client_port,fd);
			}else if(c_msg->type == CLIENT_DEL){
				status_del(c_msg->client_ip,c_msg->client_port);
			}
		}else{
			close(fd);
			select_sever_del(fd);
		}
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  interception_init
 *  Description:  init for interception
 * =====================================================================================
 */
void interception_init(){
	delay_table_init();
	status_init();
	select_sever_set_callback(interception_process);
	msg_listen_sock = msg_receiver_init();
	select_sever_add(msg_listen_sock);
	firewall_sock = nl_firewall_init();
	select_sever_add(firewall_sock);
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  interception_run
 *  Description:  main procedure for running interception
 * =====================================================================================
 */
void interception_run(){
	select_server_run();
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  interception_over
 *  Description:  interception over
 * =====================================================================================
 */
void interception_over(){
	if(firewall_sock!=-1){
		close(firewall_sock);
	}

	if(msg_listen_sock!=-1){
		close(msg_listen_sock);
	}
	status_destroy();
	delay_table_destroy();
}
