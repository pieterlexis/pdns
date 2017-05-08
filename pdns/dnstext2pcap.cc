#include "dnspcap.hh"
#include "dnswriter.hh"
#include <fstream>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <vector>

string strip(string& in)
{
    string &out = in;

    if(out[0] == ' ') out = out.substr(1, in.size()-1);
    if(out[0] == '\'' and out[out.size()-1] == '\'')
        out = out.substr(1, out.size()-2);

    return out;
}

void setaddress(struct in_addr *out, u_short *port, const string &value)
{
  string myval = value;
  u_short myport;
  std::replace( myval.begin(), myval.end(), '#', ':');
  cout<<"   setaddress got ["<<myval<<"]"<<endl;
  ComboAddress ca(myval);
  memcpy(out, &ca.sin4.sin_addr.s_addr, sizeof(ca.sin4.sin_addr.s_addr));
  myport = ca.sin4.sin_port;
  memcpy(port, &myport, sizeof(myport));
}

void setflags(dnsheader *dh, string &flags)
{
  vector<string> flagv;

  if(flags[0] != '(' || flags[flags.size()-1] != ')')
    throw runtime_error("invalid flags");
  flags = flags.substr(1, flags.size()-2);
  cout<<"  got flags: ["<<flags<<"]"<<endl;
  boost::split(flagv, flags, boost::is_any_of(" "));
  for(auto &flag: flagv) {
    cout<<"   got flag: "<<flag<<endl;
    flag = strip(flag);
    if (flag == "RD") { cout<<"    setting RD"<<endl; dh->rd=1; }
    else if(flag == "RA") { cout<<"    setting RA"<<endl; dh->ra=1; }
  }
}

int main(int argc, char **argv)
{
  pdns_pcap_file_header pfh;
  std::ifstream infile(argv[1]);
  FILE *out=fopen(argv[2], "w");

  pfh.magic = htonl(0xa1b2c3d4);
  pfh.version_major = htons(2);
  pfh.version_minor = htons(4);
  pfh.thiszone = 0;
  pfh.sigfigs = 0;
  pfh.snaplen = htonl(1500);
  pfh.linktype = htonl(101);

  fwrite(&pfh, 1, sizeof(pfh), out);

  string token;

  while(!infile.eof()) {
    uint16_t ipid = 0;
    infile >> token;
    cout<<"got token '"<<token<<"'"<<endl;
    if(token != "{") {
        throw runtime_error("packet start missing");
    }

    struct pdns_pcap_pkthdr pheader;
    struct udphdr uh;
    struct ip ih;
    DNSName qname;
    QType qtype;
    uint16_t qid;
    string flags;
    uint16_t reqsize;

    while(!infile.eof()) {
      ih.ip_v=4;
      ih.ip_hl = sizeof(ih) >> 2;
      ih.ip_tos = 0;
      // ih.ip_len
      ih.ip_id=ipid++;
      ih.ip_off=0;
      ih.ip_ttl=127;
      ih.ip_p=17;
      ih.ip_sum=0;

      string to, value;
      infile >> token;
      if(token == "}") {
        getline(infile, value); // discard
        cout<<" writing packet with qname "<<qname.toString()<<endl;
        vector<uint8_t> content;
        DNSPacketWriter pw(content, qname, qtype.getCode(), 1, 0);
        pw.getHeader()->id = htons(qid);
        setflags(pw.getHeader(), flags);
        pw.commit();
        uh.uh_ulen = htons(sizeof(uh) + content.size());
        uh.uh_sum = 0;

        ih.ip_len = htons(sizeof(ih) + sizeof(uh) + content.size());

        pheader.len = htonl(sizeof(ih) + sizeof(uh) + content.size());
        pheader.caplen = pheader.len;

        cout<<"lengths pcap/ip/udp/dns/reqsize: "<<ntohl(pheader.len)<<", "
                                                 <<ntohs(ih.ip_len)<<", "
                                                 <<ntohs(uh.uh_ulen)<<", "
                                                 <<content.size()<<", "
                                                 <<reqsize<<endl;

        if (content.size() != reqsize) throw runtime_error("content/reqsize mismatch");

        fwrite(&pheader, 1, sizeof(pheader), out);
        fwrite(&ih, 1, sizeof(ih), out);
        fwrite(&uh, 1, sizeof(uh), out);
        fwrite(content.data(), 1, content.size(), out);

        goto next;
      }
      infile >> to;
      getline(infile, value);
      value = strip(value);
      if (token == "timestamp") {
        uint32_t ts;
        ts = std::stoul(value);
        cout<<"   got timestamp "<<ts<<endl;
        pheader.ts.tv_sec = htonl(ts);
        pheader.ts.tv_usec = 0;
      } else if (token == "start-time") {
        double ts;
        uint32_t sec, usec;
        ts = std::stod(value);
        sec = ts;
        usec = 1.0e9 * (ts - sec);
        cout<<"   got start-time "<<sec<<";"<<usec<<endl;
        pheader.ts.tv_sec = htonl(sec);
        pheader.ts.tv_usec = htonl(usec);
      } else if (token == "client-address") {
        setaddress(&ih.ip_src, &uh.uh_sport, value);
      } else if (token == "local-address") {
        setaddress(&ih.ip_dst, &uh.uh_dport, value);
      } else if (token == "name") {
        qname = DNSName(value);
        cout<<"   got qname "<<value<<" parsed as "<<qname.toString()<<endl;
      } else if (token == "query-type") {
        qtype = value;
      } else if (token == "query-id") {
        qid = std::stoul(value);
      // FIXME: for rcode: can we fake up a response packet to make dnsreplay happy, even if it has no content?
      // } else if (token == "result-code") {
      //   if (value == "noerror") rcode = 0;
      //   else throw runtime_error("unknown result-code");
      } else if (token == "query-class") {
        if (value != "IN") throw runtime_error("unknown query-class");
      } else if (token == "flags") {
        flags = value;
      } else if (token == "request-size") {
        reqsize = std::stoul(value);
      } else {
        cout<<"  ignored token/to/value ["<<token<<"] ["<<to<<"] ["<<value<<"]"<<endl;
      }
    }
  next:;
  }
}
