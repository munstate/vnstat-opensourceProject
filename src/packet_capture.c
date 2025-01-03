#include "packet_capture.h"
#include "traffic.h"
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/ip.h>  // IP 헤더 정의
#include <netinet/tcp.h> // TCP 헤더 정의
#include <arpa/inet.h>   // ntohs 함수 사용
#include <stdio.h>       // 기본 출력 함수

//김태영 created file

//"창주" 주석이 있는것은 창주 lines, 주석이 없으면 태영 lines

#define MAX_CAPTURED_IPS 10 // 캡처할 서로 다른 IP 개수 제한 (창주)

static char **captured_ips = NULL; // 캡처된 IP 배열
static int captured_count = 0;     // 캡처된 서로 다른 IP의 개수

// 패킷 처리 콜백 함수
void packet_handler(u_char *user, const struct pcap_pkthdr *header, const u_char *packet) {
    struct ip *ip_header = (struct ip *)(packet + 14); // Ethernet 헤더를 건너뜀 (14바이트)
    struct tcphdr *tcp_handler = (struct tcphdr *)(packet + 14 + (ip_header->ip_hl << 2)); //필터링 된 패킷 처리 (창주)
    
    // 발신 IP 추출
    if (ip_header->ip_src.s_addr == 0xC0A80001 && ntohs(tcp_handler->th_dport) == 80) { // 192.168.0.1:80
        printf("Captured packet from IP 192.168.0.1 and port 80\n");
    } //창주 3

    const char *source_ip = inet_ntoa(ip_header->ip_src);
    if (!source_ip) {
        fprintf(stderr, "Error: Failed to extract source IP.\n");
        return;
    }
    printf("Source IP: %s\n", source_ip);

    // 이미 저장된 IP인지 확인
    for (int i = 0; i < captured_count; i++) {
        if (strcmp(captured_ips[i], source_ip) == 0) {
            return; // 중복 IP는 무시
        }
    }

    // 새로운 IP 저장
    if (captured_count < MAX_CAPTURED_IPS) {
        captured_ips[captured_count] = strdup(source_ip); // 동적 메모리 할당
        if (!captured_ips[captured_count]) {
            fprintf(stderr, "Error: Memory allocation failed for IP storage.\n");
            return;
        }
        printf("Captured unique IP #%d: %s\n", captured_count + 1, captured_ips[captured_count]);
        captured_count++;
    }

    //서로 다른 IP를 캡처했으면 pcap_loop 종료
    if (captured_count >= MAX_CAPTURED_IPS) {
        pcap_breakloop((pcap_t *)user); // pcap_breakloop 호출
    }
}

// 패킷 캡처 시작 함수
const char** start_packet_capture(const char *dev, int *out_count) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;

    // 초기화
    captured_ips = (char **)malloc(MAX_CAPTURED_IPS * sizeof(char *));
    if (!captured_ips) {
        fprintf(stderr, "Error: Failed to allocate memory for IP array.\n");
        return NULL;
    }
    memset(captured_ips, 0, MAX_CAPTURED_IPS * sizeof(char *));

    captured_count = 0;

    // 네트워크 인터페이스 열기
    handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Could not open device %s: %s\n", dev, errbuf);
        free(captured_ips); // 메모리 해제
        captured_ips = NULL;
        return NULL;
    }

    // 패킷 캡처 시작
    printf("Starting packet capture on device: %s\n", dev);
    pcap_loop(handle, 0, packet_handler, (u_char *)handle); // handle을 사용자 데이터로 전달

    // 캡처 종료
    pcap_close(handle);

    // 캡처된 IP의 개수를 반환
    *out_count = captured_count;

    // 캡처된 IP가 없으면 NULL 반환
    if (captured_count == 0) {
        free(captured_ips);
        captured_ips = NULL;
        return NULL;
    }

    return (const char **)captured_ips;
}

void handle_packet_capture(const char *dev) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);

    if (!handle) {
        fprintf(stderr, "Error opening device %s: %s\n", dev, errbuf);
        return;
    }

    const u_char *packet;
    struct pcap_pkthdr header;
    struct ip *ip_header;

    while ((packet = pcap_next(handle, &header)) != NULL) {
        struct ip *ip_header = (struct ip *)(packet + 14); // Ethernet 헤더는 14바이트
        char src_ip[INET_ADDRSTRLEN];
        char dst_ip[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &ip_header->ip_src, src_ip, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &ip_header->ip_dst, dst_ip, INET_ADDRSTRLEN);

        monitor_ip_traffic(src_ip, 0, header.len); // 송신 트래픽
        monitor_ip_traffic(dst_ip, header.len, 0); // 수신 트래픽
    }

    pcap_close(handle);
}//창주 105~131

// 캡처된 IP 메모리 해제 함수
void free_captured_ips() {
    if (captured_ips) {
        for (int i = 0; i < captured_count; i++) {
            if (captured_ips[i]) {
                free(captured_ips[i]);
            }
        }
        free(captured_ips);
        captured_ips = NULL;
    }
}
//나머지 태영
