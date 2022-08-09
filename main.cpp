#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include "include/Rtklib/rtklib_fun.h"
using namespace std;
using namespace bamboo;
#define VEL_LIGHT 299792458.0
class SSRCLK
{
public:
    SSRCLK() { m_reset(); }
    void m_reset()
    {
        int isat;
        wk = 0;
        sow = 0.0;
        memset(c, 0, sizeof(double) * MAXRTKSAT * 3);
        for (isat = 0; isat < MAXRTKSAT; isat++)
            iod[isat] = -1;
    }
    int wk;
    double sow;
    int iod[MAXRTKSAT];
    double c[MAXRTKSAT][3];
};
class SSRORB
{
public:
    SSRORB() { m_reset(); }
    void m_reset()
    {
        int isat;
        wk = 0;
        sow = 0.0;
        memset(dx, 0, sizeof(double) * MAXRTKSAT * 6);
        for (isat = 0; isat < MAXRTKSAT; isat++)
            iod[isat] = -1;
    }
    int wk;
    double sow;
    int iod[MAXRTKSAT];
    double dx[MAXRTKSAT][6];
};
void printEphemeris(nav_t *nav, int psat, int offset)
{
    double toe;
    int week;
    char prn[256] = {0};
    satno2id(psat, prn);
    if (prn[0] != 'R')
    {
        psat = psat + offset;
        double toe = time2gpst(nav->eph[psat - 1].toe, &week);
        /// the ephemeris is restored in nav->eph[psat-1]
        printf("receving ephmeris: %s %d %9.1lf\n", prn, week, toe);
    }
    else
    {
        /// the ephemeris is restored in nav->geph[psat-1]
        psat = atoi(prn + 1);
        toe = time2gpst(nav->geph[psat - 1].toe, &week); /*GPST*/
        printf("receving GLONASS ephemeris: %s %d %9.1lf\n", prn, week, toe);
    }
}
void printOrbitClk(ssr_t *ssr)
{
    double sow;
    int isat, ueph = false, uclk = false, ret = 0;
    char cprn[256] = {0};
    SSRORB orb;
    SSRCLK clk;
    /*only support orbit & clock */
    for (isat = 0; isat < MAXRTKSAT; isat++)
    {
        if (ssr[isat].update)
        {
            ssr[isat].update = 0;
            satno2id(isat + 1, cprn);

            if (ssr[isat].iod[0] != -999)
            {
                ueph = true;
                ssr[isat].iod[0] = -999;
                /*fill the value*/
                orb.sow = time2gpst(ssr[isat].t0[0], &orb.wk);
                orb.iod[isat] = cprn[0] == 'C' ? ssr[isat].iodcrc : ssr[isat].iode;
                memcpy(orb.dx[isat], ssr[isat].deph, sizeof(double) * 3);
                memcpy(orb.dx[isat] + 3, ssr[isat].ddeph, sizeof(double) * 3);
            }
            if (ssr[isat].iod[1] != -999)
            {
                uclk = true;
                ssr[isat].iod[1] = -999;
                /*fill the value*/
                clk.sow = time2gpst(ssr[isat].t0[1], &clk.wk);
                clk.iod[isat] = cprn[0] == 'C' ? ssr[isat].iodcrc : ssr[isat].iode;
                memcpy(clk.c[isat], ssr[isat].dclk, sizeof(double) * 3);
            }
        }
    }

    if (ueph && uclk)
    {
        for (int i = 0; i < MAXRTKSAT; ++i)
        {
            if (clk.iod[i] != -1 && orb.iod[i] != -1)
            {
                satno2id(i + 1, cprn);
                printf("[   ORBCLK] %04d %13.3lf %s orb: %9.3lf %9.3lf %9.3lf iode: %5d c: %9.3lf idoe: %5d\n", clk.wk, clk.sow, cprn, orb.dx[i][0],
                       orb.dx[i][1], orb.dx[i][2], orb.iod[i], clk.c[i][0], clk.iod[i]);
            }
        }
    }
    else if (ueph)
    {
    }
    else if (uclk)
    {
    }
}
void printUpdIfpb(ssr_t *ssr)
{
    double sow;
    int isat, isys, wk, ifreq;
    char cprn[256] = {0}, obstype[256] = {0};
    double phbias[MAXRTKSAT][MAXRTKFREQ] = {0};
    double *p_b = (double *)phbias;
    for (int i = 0; i < sizeof(phbias) / sizeof(double); ++i)
        p_b[i] = 999;
    for (isat = 0; isat < MAXRTKSAT; isat++)
    {
        satno2id(isat + 1, cprn);
        if (ssr[isat].iod[5] != -999)
        {
            /* update the phase-bias here */
            ssr[isat].iod[5] = -999;
            sow = time2gpst(ssr[isat].t0[5], &wk);
            char p_obsstr[1024] = {0};
            for (int i = 0; i < MAXCODE; ++i)
            {
                strcpy(obstype, code2obs(i + 1));
                if (!strlen(obstype))
                    continue;
                if (ssr[isat].stdpb[i] != 0.0)
                {
                    sprintf(p_obsstr + strlen(p_obsstr), "%s %9.3lf ", (string("L") + obstype).c_str(), ssr[isat].pbias[i]); /// unit in meters,should change into cycles by `pbias / lam`
                }
            }
            if (strlen(p_obsstr))
                printf("[PHASEBIAS] %04d %13.3lf %s pbias: %-100s yangle: %9.3lf yrate:%9.4lf \n", wk, sow, cprn,
                       p_obsstr, ssr[isat].yaw_ang > 180 ? ssr[isat].yaw_ang - 360 : ssr[isat].yaw_ang, ssr[isat].yaw_rate);
        }
        if (ssr[isat].iod[4] != -999)
        {
            ssr[isat].iod[4] = -999;
            sow = time2gpst(ssr[isat].t0[4], &wk);
            char p_obsstr[1024] = {0};
            for (int i = 0; i < MAXCODE; ++i)
            {

                strcpy(obstype, code2obs(i + 1));

                if (!strlen(obstype))
                    continue;
                if (ssr[isat].cbias[i] != 0)
                {
                    sprintf(p_obsstr + strlen(p_obsstr), "%s %9.3lf ", (string("L") + obstype).c_str(), ssr[isat].cbias[i]);
                }
            }
            printf("[ CODEBIAS] %04d %13.3lf %s cbias: %-100s\n", wk, sow, cprn, p_obsstr);
        }

        if (ssr[isat].iod[6] != -999)
        {
            int wk_t0, mjd;
            double sow_t0, sod;
            ssr[isat].iod[6] = -999;
            sow = time2gpst(ssr[isat].t0[6], &wk);                                                                  // now
            sow_t0 = time2gpst(ssr[isat].ifpb_t0, &wk_t0);                                                          // accumulated delay started from the benchmark time
            printf("[     IFPB] %04d %13.3lf %s ifpb: %9.3lf t0:%13.3lf\n", wk, sow, cprn, ssr[isat].ifpb, sow_t0); /// ifpb in cycles, the GPS L5 carrier measurements should directly add this corrections, if sow_t0 is different, new ambiguity should be set
        }
    }
}
static void *s_pthReceving(void *parg)
{
    int ret;
    rtcm_t rtcm;
    /// step 1, variables initializer
    init_rtcm(&rtcm);
    char addr[256] = {0}, port_str[256] = {0}, mnt[256] = {0}, buff[1024] = {0}, *path = (char *)parg;
    stream_t stream;
    /// step 2, connect to server
    strinit(&stream);
    // stropen(&stream, STR_TCPCLI, STR_MODE_RW, path);
    stropen(&stream, STR_NTRIPCLI, STR_MODE_RW, path);
    strsettimeout(&stream, 60000, 10000); /// 60s for timeout 10s for reconnect
    /// step 3, loop to decode binary data
    while (true)
    {
        int nread = strread(&stream, (unsigned char *)buff, 1024);
        for (int i = 0; i < nread; ++i)
        {
            switch ((ret = input_rtcm3(&rtcm, (unsigned char)buff[i])))
            {
            case 2:
                //// ephemeris data
                printEphemeris(&rtcm.nav, rtcm.ephsat, rtcm.ephset ? MAXRTKSAT : 0);
                break;
            case 10:
                /* acquire the corresponding ssr data*/
                printOrbitClk(rtcm.ssr);
                break;
            case 20:
                /* acquire the corresponding ssr data*/
                printUpdIfpb(rtcm.ssr);
                break;
            }
        }
    }
    strclose(&stream);
    free_rtcm(&rtcm);
    return 0;
}
int main(int argc, char *args[])
{
    pthread_t pid;
    char c_pth[256] = {"usr:psd@103.143.19.54:2101/RTCM32SSR-COM"}; // or using ntrip fmt: user:passed@ip/mnt, stropen should choose STR_NTRIPCLI
    if (0 != pthread_create(&pid, NULL, &s_pthReceving, (void *)c_pth))
    {
        cout << "ERROR(RnxEphStreamAdapter):v_openRnxEph create thread error!" << endl;
        exit(1);
    }
    while (true)
    {
        usleep(1e6);
    }
}
