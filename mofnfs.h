//
//  mofnfs.h
//  libjl777
//
//  Created by jl777 on 10/6/14.
//  Copyright (c) 2014 jl777. MIT license
//

#ifndef libjl777_mofnfs_h
#define libjl777_mofnfs_h

/*
 compression
 combinatorics on errors
 metadata in a file(s)
 persistence
 queue restore task
 teleport accounting
 message API
 sendfile
 dropout of server detected -> data shuffle?
 */

char *onion_sendfile(int32_t L,struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *dest,FILE *fp)
{
    return(0);
}

char **gen_privkeys(int32_t **cipheridsp,char *name,char *password,char *keygen)
{
    long i,len;
    uint64_t passwordhash,namehash;
    char key[64],**privkeys = 0;
    *cipheridsp = 0;
    if ( password != 0 && password[0] != 0 )
    {
        namehash = calc_txid((unsigned char *)name,(int32_t)strlen(name));
        len = strlen(password);
        passwordhash = (namehash ^ calc_txid((unsigned char *)keygen,(int32_t)strlen(keygen)) ^ calc_txid((unsigned char *)password,(int32_t)len));
        privkeys = calloc(len+1,sizeof(*privkeys));
        (*cipheridsp) = calloc(len+1,sizeof(*cipheridsp));
        for (i=0; i<len; i++)
        {
            expand_nxt64bits(key,passwordhash);
            (*cipheridsp)[i] = (password[i] % NUM_CIPHERS);
            privkeys[i] = clonestr(key);
            if ( i < len-1 )
                passwordhash ^= (namehash ^ calc_txid((unsigned char *)key,(int32_t)strlen(key)));
        }
    }
    return(privkeys);
}

uint64_t mofn_calcdatastr(char *usbdir,char *datastr,char **privkeys,int32_t *cipherids,unsigned char *data,int32_t len)
{
    FILE *fp;
    uint64_t keyhash;
    unsigned char *final,*encoded = 0;
    int32_t newlen;
    char fragname[512];
    if ( privkeys != 0 )
    {
        newlen = (int32_t)len;
        encoded = ciphers_codec(0,privkeys,cipherids,data,&newlen);
        final = encoded;
    }
    else
    {
        final = data;
        newlen = (int32_t)len;
    }
    init_hexbytes_noT(datastr,final,newlen);
    keyhash = calc_txid(final,newlen);
    if ( usbdir != 0 && usbdir[0] != 0 )
    {
        sprintf(fragname,"%s/%llu",usbdir,(long long)keyhash);
        if ( (fp= fopen(fragname,"wb")) != 0 )
        {
            if ( fwrite(final,1,newlen,fp) != newlen )
                printf("fwrite len.%d error for %llu\n",newlen,(long long)keyhash);
            else printf("data.%p wrote len.%d (%x) -> %s\n",data,newlen,*(int *)final,fragname);
            fclose(fp);
        }
    }
    if ( encoded != 0 )
        free(encoded);
    return(keyhash);
}

int32_t verify_fragment(char *usbdir,uint64_t txid,unsigned char *fragment,int32_t len)
{
    long val,z;
    FILE *checkfp;
    unsigned char buf[4096];
    char fragname[512];
    sprintf(fragname,"%s/%llu",usbdir,(long long)txid);
    if ( (checkfp= fopen(fragname,"rb")) != 0 )
    {
        memset(buf,0,len);
        if ( (val= fread(buf,1,len,checkfp)) != len )
            printf("fread.%ld len.%d error for %llu\n",val,len,(long long)txid);
        fseek(checkfp,0,SEEK_END);
        if ( ftell(checkfp) != len )
            printf("checksize %ld != len.%d for fragment.%s\n",ftell(checkfp),len,fragname);
        fclose(checkfp);
        if ( memcmp(buf,fragment,len) != 0 )
        {
            for (z=0; z<len; z++)
                if ( buf[z] != fragment[z] )
                    printf("%ld.(%02x:%02x) ",z,buf[z],fragment[z]);
            printf("memcmp error for fragment.(%s) len.%d\n",fragname,len);
            return(-1);
        } else printf("VERIFIED.%llu %s\n",(long long)txid,fragname);
    } else { printf("cant find checkfile.(%s) len.%d\n",fragname,len); return(-2); }
    return(0);
}

void calc_shares(unsigned char *shares,unsigned char *secret,int32_t size,int32_t width,int32_t M,int32_t N,unsigned char *sharenrs);
void gfshare_extract(unsigned char *secretbuf,uint8_t *sharenrs,int32_t N,uint8_t *buffer,int32_t len,int32_t size);

char *mofn_restorefile(struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,FILE *fp,int32_t L,int32_t M,int32_t N,char *usbdir,char *password,char *filename,char *sharenrsstr,uint64_t *txids)
{
    long i,len;
    cJSON *json;
    uint64_t txid;
    double endmilli = milliseconds() + 30000;
    int32_t j,m,n,hwmgood,newlen,good,*cipherids,*lengths,status = 0;
    unsigned char buf[4096],sharenrs[255],refnrs[255],*buffer=0,*decoded,**fragments = 0;
    char retstr[4096],key[64],datastr[8192],*str,**privkeys = 0;
    if ( N <= 0 )
        return(0);
    memset(refnrs,0,sizeof(refnrs));
    memset(sharenrs,0,sizeof(sharenrs));
    if ( N > 1 )
    {
        len = strlen(sharenrsstr);
        if ( len == N*2 )
            decode_hex(refnrs,(int32_t)len/2,sharenrsstr);
        else
        {
            sprintf(retstr,"{\"error\":\"MISMATCHED sharenrs.(%s) vs M.%d of N.%d\"}",sharenrsstr,M,N);
            printf("mofn_restorefile ERROR.(%s)\n",retstr);
            return(clonestr(retstr));
        }
    }
    for (n=0; txids[n]!=0; n++)
        ;
    if ( (n % N) != 0 )
    {
        sprintf(retstr,"{\"error\":\"wrong number of txids %d mod N.%d is %d\"}",n,N,n%N);
        printf("mofn_restorefile ERROR.(%s)\n",retstr);
        return(clonestr(retstr));
    }
    lengths = calloc(n,sizeof(*lengths));
    fragments = calloc(n,sizeof(*fragments));
    privkeys = gen_privkeys(&cipherids,filename,password,NXTACCTSECRET);
    hwmgood = 0;
    while ( milliseconds() < endmilli )
    {
        for (i=0; i<n; i++)
        {
            if ( fragments[i] != 0 )
                continue;
            expand_nxt64bits(key,txids[i]);
            str = kademlia_find("findvalue",prevaddr,verifiedNXTaddr,NXTACCTSECRET,sender,0,key);
            if ( str != 0 )
            {
                if ( (json= cJSON_Parse(str)) != 0 )
                {
                    copy_cJSON(datastr,cJSON_GetObjectItem(json,"data"));
                    len = strlen(datastr);
                    if ( len >= 2 && (len & 1) == 0 && is_hexstr(datastr) != 0 )
                    {
                        len >>= 1;
                        fragments[i] = calloc(1,len);
                        lengths[i] = (int32_t)len;
                        decode_hex(fragments[i],(int32_t)len,datastr);
                        if ( usbdir != 0 && usbdir[0] != 0 )
                            verify_fragment(usbdir,txids[i],fragments[i],lengths[i]);
                        if ( (txid= calc_txid(fragments[i],lengths[i])) != txids[i] )
                        {
                            printf("txid error for fragment.%ld len.%d %llu != %llu\n",i,lengths[i],(long long)txid,(long long)txids[i]);
                            lengths[i] = 0;
                        }
                        else
                        {
                            if ( privkeys != 0 )
                            {
                                newlen = lengths[i];
                                decoded = ciphers_codec(1,privkeys,cipherids,fragments[i],&newlen);
                                if ( decoded != 0 )
                                {
                                    free(fragments[i]);
                                    fragments[i] = decoded;
                                    lengths[i] = newlen;
                                } else printf("decode ciphers error fragment.%ld len.%d\n",i,newlen);
                            }
                        }
                    } else printf("not hex or badlen.%ld\n",len);
                    free_json(json);
                }
                //printf("i.%ld sharei.%d of %d: %s\n",i,sharei,N,str);
                free(str);
            }
        }
        good = 0;
        for (i=0; i<n; i+=N)
        {
            len = 0;
            for (j=m=0; j<N; j++)
            {
                if ( fragments[i+j] != 0 && lengths[i+j] != 0 )
                {
                    if ( len == 0 )
                        len = lengths[i+j];
                    if ( len != lengths[i+j] )
                        printf("length mismatch error: fragment.%ld/%d len.%ld != %d\n",i/N,j,len,lengths[i+j]);
                    else m++;
                }
            }
            //printf("m.%d M.%d n.%d\n",m,M,n);
            if ( m >= M )
                good++;
        }
        if ( good > hwmgood )
        {
            printf("hwmgood.%d -> %d extend time\n",hwmgood,good);
            hwmgood = good;
            endmilli += 10000;
        }
        if ( good == n/N )
        {
            printf("good.%d vs n.%d\n",good,n/N);
            for (i=0; i<n; i+=N)
            {
                len = 0;
                buffer = 0;
                for (j=m=0; j<N; j++)
                {
                    if ( fragments[i+j] != 0 )
                    {
                        if ( len == 0 )
                        {
                            len = lengths[i+j];
                            buffer = calloc(N,len);
                        }
                        if ( len != lengths[i+j] )
                            printf("length mismatch error: fragment.%ld/%d len.%ld != %d\n",i/N,j,len,lengths[i+j]);
                        else
                        {
                            memcpy(buffer + j*len,fragments[i+j],len);
                            m++;
                            sharenrs[j] = refnrs[j];
                            if ( m == M )
                                break;
                        }
                    }
                }
                if ( buffer != 0 )
                {
                    if ( N > 1 )
                    {
                        memset(buf,0,sizeof(buf));
                        //int z;
                        //for (z=0; z<N*len; z++)
                        //    printf("%02x",buffer[z]);
                        gfshare_extract(buf,sharenrs,N,buffer,(int32_t)len,(int32_t)len);
                        printf(" RESTORED.(%x) len.%ld sharenrs.(%08x)\n",*(int *)buf,len,*(int *)sharenrs);
                        if ( fwrite(buf,1,len,fp) != len )
                            printf("write.%ld error in m.%d/%d of N.%d fragment.%ld of %d\n",len,m,M,N,i,n);
                    }
                    else
                    {
                        if ( fwrite(buffer,1,len,fp) != len )
                            printf("write.%ld error in fragment.%ld of %d\n",len,i,n);
                    }
                    free(buffer);
                }
            }
            break;
        }
        sleep(3);
        printf("restorefile.(%s): have %d of %d fragments | %.2f seconds left\n",filename,good,n/N,(endmilli - milliseconds())/1000.);
    }
    status = (hwmgood - n/N);
    if ( privkeys != 0 )
    {
        for (i=0; privkeys[i]!=0; i++)
            free(privkeys[i]);
        free(privkeys);
        free(cipherids);
    }
    if ( fragments != 0 )
    {
        for (i=0; i<n; i++)
            if ( fragments[i] != 0 )
                free(fragments[i]);
        free(fragments);
        free(lengths);
    }
    sprintf(retstr,"{\"result\":\"status.%d\",\"completed\":%.3f,\"filesize\":\"%ld\",\"descr\":\"mofn_restorefile M.%d of N.%d sent with Lfactor.%d usbname.(%s) usedpassword.%d reconstructed\"}",status,(double)hwmgood/(n/N),ftell(fp),M,N,L,usbdir,password[0]!=0);
    
    printf("RESTORE.(%s)\n",retstr);
    return(clonestr(retstr));
}

char *mofn_savefile(struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,FILE *fp,int32_t L,int32_t M,int32_t N,char *usbdir,char *password,char *filename)
{
    long i,len;
    FILE *savefp;
    uint64_t keyhash,txids[1000];
    cJSON *array;
    int32_t n,sharei,err,*cipherids=0,status = 0;
    unsigned char buf[1024],sharenrs[255],*buffer;
    char *retstr,savefname[512],key[64],datastr[sizeof(buf)*3+1],*str,**privkeys = 0;
    i = n = 0;
    array = cJSON_CreateArray();
    privkeys = gen_privkeys(&cipherids,filename,password,NXTACCTSECRET);
    memset(sharenrs,0,sizeof(sharenrs));
    init_sharenrs(sharenrs,0,N,N);
    while ( (len= fread(buf,1,sizeof(buf),fp)) > 0 )
    {
        if ( N > 1 )
        {
            buffer = calloc(N,len);
            calc_shares(buffer,buf,(int32_t)len,(int32_t)len,M,N,sharenrs);
            if ( 0 )
            {
                int z;
                unsigned char buf2[4096];
                memset(buf2,0,sizeof(buf2));
                gfshare_extract(buf2,sharenrs,N,buffer,(int32_t)len,(int32_t)len);
                for (z=0; z<N*len; z++)
                    printf("%02x",buffer[z]);
                for (z=0; z<len; z++)
                    printf(" %02x:%02x",buf[z],buf2[z]);
                printf("cmp.%d RESTORED.(%x) len.%ld sharenrs.(%08x)\n",memcmp(buf,buf2,len),*(int *)buf2,len,*(int *)sharenrs);
            }
            for (sharei=err=0; sharei<N; sharei++)
            {
                memset(datastr,0,sizeof(datastr));
                keyhash = mofn_calcdatastr(usbdir,datastr,privkeys,cipherids,buffer + sharei*len,(int32_t)len);
                expand_nxt64bits(key,keyhash);
                if ( n < sizeof(txids)/sizeof(*txids) )
                    txids[n] = keyhash;
                n++;
                cJSON_AddItemToArray(array,cJSON_CreateString(key));
                fprintf(stderr,"%s ",key);
                str = kademlia_storedata(prevaddr,verifiedNXTaddr,NXTACCTSECRET,sender,0,key,datastr);
                if ( str != 0 )
                {
                    //printf("i.%ld sharei.%d of %d: %s\n",i,sharei,N,str);
                    free(str);
                }
            }
            free(buffer);
        }
        else
        {
            keyhash = mofn_calcdatastr(usbdir,datastr,privkeys,cipherids,buf,(int32_t)len);
            if ( n < sizeof(txids)/sizeof(*txids) )
                txids[n] = keyhash;
            n++;
            expand_nxt64bits(key,keyhash);
            cJSON_AddItemToArray(array,cJSON_CreateString(key));
            printf("%s ",key);
            str = kademlia_storedata(prevaddr,verifiedNXTaddr,NXTACCTSECRET,sender,0,key,datastr);
            if ( str != 0 )
            {
                //printf("i.%ld: %s\n",i,str);
                free(str);
            }
        }
    }
    if ( privkeys != 0 )
    {
        for (i=0; privkeys[i]!=0; i++)
            free(privkeys[i]);
        free(privkeys);
        free(cipherids);
    }
    if ( N > 1 )
        init_hexbytes_noT(datastr,sharenrs,N);
    else datastr[0] = 0;
    str = cJSON_Print(array);
    stripwhite_ns(str,strlen(str));
    free_json(array);
    //printf("strlen.%ld (%s)\n",strlen(str),str);
    retstr = calloc(1,strlen(str)+4096);
    if ( usbdir != 0 )
    {
        sprintf(savefname,"%s/%s",usbdir,filename);
        if ( (savefp= fopen(savefname,"wb")) != 0 )
        {
            sprintf(retstr,"./BitcoinDarkd SuperNET '{\"requestType\":\"restorefile\",\"filename\":\"%s\",\"L\":%d,\"M\":%d,\"N\":%d,\"usbdir\":\"%s\",\"destfile\":\"%s.restored\",\"sharenrs\":\"%s\",\"txids\":%s%s}'",filename,L,M,N,usbdir,filename,datastr,str,password[0]!=0?",\"password\":yourPIN":"");
            fprintf(savefp,"%s\n",retstr);
            printf("%s\n",retstr);
            fclose(savefp);
        }
    }
    else
    {
        sprintf(retstr,"./BitcoinDarkd SuperNET '{\"requestType\":\"restorefile\",\"filename\":\"%s\",\"L\":%d,\"M\":%d,\"N\":%d,\"destfile\":\"%s.restored\",\"sharenrs\":\"%s\",\"txids\":%s%s}'",filename,L,M,N,filename,datastr,str,password[0]!=0?",\"password\":yourPIN":"");
        printf("%s\n",retstr);
    }
    sprintf(retstr,"{\"result\":\"status.%d\",\"sharenrs\":\"%s\",\"txids\":%s,\"filesize\":\"%ld\",\"descr\":\"mofn_savefile M.%d of N.%d sent with Lfactor.%d usbdir.(%s) usedpassword.%d dont lose the password, sharenrs or txids!\"}",status,datastr,str,ftell(fp),M,N,L,usbdir,password[0]!=0);
    //printf("SAVE.(%s)\n",retstr);
    free(str);
    if ( 0 && n < sizeof(txids)/sizeof(*txids) )
    {
        FILE *fp = fopen("foo.txt.restore","wb");
        txids[n] = 0;
        mofn_restorefile(0,verifiedNXTaddr,NXTACCTSECRET,sender,fp,L,M,N,usbdir,password,filename,datastr,txids);
        fclose(fp);
        char cmdstr[512];
        printf("\n*****************\ncompare (%s) vs (%s)\n",filename,"foo.txt.restore");
        sprintf(cmdstr,"cmp %s %s",filename,"foo.txt.restore");
        system(cmdstr);
        printf("done\n\n");
    }
    return(retstr);
}

double calc_address_metric(int32_t dispflag,uint64_t refaddr,uint64_t *list,int32_t n,uint64_t calcaddr,int32_t targetdist)
{
    int32_t i,numabove,numbelow,exact,flag = 0;
    double metric,dist,diff,sum,balance;
    metric = bitweight(refaddr ^ calcaddr);
    if ( metric > targetdist+4 )
        return(10000000.);
    exact = 0;
    diff = sum = balance = 0.;
    if ( list != 0 && n != 0 )
    {
        numabove = numbelow = 0;
        for (i=0; i<n; i++)
        {
            if ( list[i] != refaddr )
            {
                dist = bitweight(list[i] ^ calcaddr);
                if ( dist > metric )
                    numabove++;
                else if ( dist < metric )
                    numbelow++;
                else exact++;
                if ( dispflag > 1 )
                    printf("(%llx %.0f) ",(long long)list[i],dist);
                else if ( dispflag != 0 )
                    printf("%.0f ",dist);
                sum += (dist * dist);
                dist -= metric;
                diff += (dist * dist);
            } else flag = 1;
        }
        if ( n == 1 )
            flag = 0;
        balance = fabs(numabove - numbelow);
        balance *= balance * 10;
        sum = sqrt(sum / (n - flag));
        diff = sqrt(diff / (n - flag));
        if ( dispflag != 0 )
            printf("n.%d flag.%d sum %.3f | diff %.3f | exact.%d above.%d below.%d balance %.0f ",n,flag,sum,diff,exact,numabove,numbelow,balance);
    }
    if ( dispflag != 0 )
        printf("dist %.3f -> %.3f %llx %llu\n",metric,(diff + balance)/(exact*exact+1),(long long)refaddr,(long long)refaddr);
    return((cbrt(metric) + diff + balance)/(exact*exact+1));
}

struct loopargs
{
    char refacct[256],bestpassword[4096];
    double best;
    uint64_t bestaddr,*list;
    int32_t abortflag,threadid,numinlist,targetdist,numthreads,duration;
};

void *findaddress_loop(void *ptr)
{
    struct loopargs *args = ptr;
    uint64_t addr,calcaddr;
    int32_t i,n=0;
    double startmilli,metric;
    unsigned char hash[256 >> 3],mypublic[256>>3],pass[49];
    addr = calc_nxt64bits(args->refacct);
    n = 0;
    startmilli = milliseconds();
    while ( args->abortflag == 0 )
    {
        if ( 0 )
        {
            //memset(pass,0,sizeof(pass));
            //randombytes(pass,(sizeof(pass)/sizeof(*pass))-1);
            for (i=0; i<(int)(sizeof(pass)/sizeof(*pass))-1; i++)
            {
                //if ( pass[i] == 0 )
                pass[i] = safechar64((rand() >> 8) % 63);
            }
            pass[i] = 0;
            memset(hash,0,sizeof(hash));
            memset(mypublic,0,sizeof(mypublic));
            calcaddr = conv_NXTpassword(hash,mypublic,(char *)pass);
        }
        else randombytes((unsigned char *)&calcaddr,sizeof(calcaddr));
        if ( bitweight(addr ^ calcaddr) <= args->targetdist+4 )
        {
            metric = calc_address_metric(0,addr,args->list,args->numinlist,calcaddr,args->targetdist);
            if ( metric < args->best )
            {
                metric = calc_address_metric(1,addr,args->list,args->numinlist,calcaddr,args->targetdist);
                args->best = metric;
                args->bestaddr = calcaddr;
                strcpy(args->bestpassword,(char *)pass);
                printf("thread.%d n.%d: best.%.4f -> %llu | %llu calcaddr | ave micros %.3f\n",args->threadid,n,args->best,(long long)args->bestaddr,(long long)calcaddr,1000*(milliseconds()-startmilli)/n);
            }
        }
        n++;
    }
    args->abortflag = -1;
    return(0);
}

char *findaddress(struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,uint64_t addr,uint64_t *list,int32_t n,int32_t targetdist,int32_t duration,int32_t numthreads)
{
    static double lastbest,endmilli,best,metric;
    static uint64_t calcaddr,bestaddr = 0;
    static char refNXTaddr[64],retbuf[2048],bestpassword[512],bestNXTaddr[64];
    static struct loopargs **args;
    bits256 secret,pubkey;
    int32_t i;
    if ( endmilli == 0. )
    {
        if ( numthreads <= 0 )
            return(0);
        expand_nxt64bits(refNXTaddr,addr);
        if ( numthreads > 28 )
            numthreads = 28;
        best = lastbest = 1000000.;
        bestpassword[0] = bestNXTaddr[0] = 0;
        args = calloc(numthreads,sizeof(*args));
        for (i=0; i<numthreads; i++)
        {
            args[i] = calloc(1,sizeof(*args[i]));
            strcpy(args[i]->refacct,refNXTaddr);
            args[i]->threadid = i;
            args[i]->numthreads = numthreads;
            args[i]->targetdist = targetdist-(i%5);
            args[i]->best = lastbest;
            args[i]->list = list;
            args[i]->numinlist = n;
            if ( portable_thread_create((void *)findaddress_loop,args[i]) == 0 )
                printf("ERROR hist findaddress_loop\n");
        }
        endmilli = milliseconds() + (duration * 1000.);
    }
    else
    {
        addr = calc_nxt64bits(args[0]->refacct);
        list = args[0]->list;
        n = args[0]->numinlist;
        targetdist = args[0]->targetdist;
        numthreads = args[0]->numthreads;
        duration = args[0]->duration;
    }
    //if ( milliseconds() < endmilli )
    {
        best = lastbest;
        calcaddr = 0;
        for (i=0; i<numthreads; i++)
        {
            if ( args[i]->best < best )
            {
                if ( args[i]->bestpassword[0] != 0 )
                    calcaddr = conv_NXTpassword(secret.bytes,pubkey.bytes,args[i]->bestpassword);
                else calcaddr = args[i]->bestaddr;
                //printf("(%llx %f) ",(long long)calcaddr,args[i]->best);
                metric = calc_address_metric(1,addr,list,n,calcaddr,targetdist);
                //printf("-> %f, ",metric);
                if ( metric < best )
                {
                    best = metric;
                    bestaddr = calcaddr;
                    if ( calcaddr != args[i]->bestaddr )
                        printf("error calcaddr.%llx vs %llx\n",(long long)calcaddr,(long long)args[i]->bestaddr);
                    expand_nxt64bits(bestNXTaddr,calcaddr);
                    strcpy(bestpassword,args[i]->bestpassword);
                }
            }
        }
        //printf("best %f lastbest %f %llu\n",best,lastbest,(long long)addr);
        if ( best < lastbest )
        {
            printf(">>>>>>>>>>>>>>> new best (%s) %016llx %llu dist.%d metric %.2f vs %016llx %llu\n",bestpassword,(long long)calcaddr,(long long)calcaddr,bitweight(addr ^ bestaddr),best,(long long)addr,(long long)addr);
            lastbest = best;
        }
        //printf("milli %f vs endmilli %f\n",milliseconds(),endmilli);
    }
    if ( milliseconds() >= endmilli )
    {
        for (i=0; i<numthreads; i++)
            args[i]->abortflag = 1;
        for (i=0; i<numthreads; i++)
        {
            while ( args[i]->abortflag != -1 )
                sleep(1);
            free(args[i]);
        }
        free(args);
        args = 0;
        metric = calc_address_metric(2,addr,list,n,bestaddr,targetdist);
        free(list);
        endmilli = 0;
        sprintf(retbuf,"{\"result\":\"metric %.3f\",\"privateaddr\":\"%s\",\"password\":%s\",\"dist\":%d,\"targetdist\":%d}",best,bestNXTaddr,bestpassword,bitweight(addr ^ bestaddr),targetdist);
        printf("FINDADDRESS.(%s)\n",retbuf);
        return(clonestr(retbuf));
    }
    return(0);
}
#endif
