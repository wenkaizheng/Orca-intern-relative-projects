//
// Created by wenkai on 1/4/21.
//
#include "utils.hpp"
char search_syms[7] = {0x1,0x2,0x3,0x4,0x5,0x6,0x0};
char remove_msg[3] = {11,12,13};
char first_msg[3] = {8,9,10};
char remove_msg_owner[3] = {14,15,16};
char owner_msg[3] = {19,20,21};
char logs[32] = "client_log.txt";
char* current_time(){
    time_t tt;
    struct tm* ti;
    time(&tt);
    ti = localtime(&tt);
    return asctime(ti);
}
static void write_complete(int fd, char* buff, size_t len){
       size_t diff = len;
       while (1){
           ssize_t complete =  write(fd,buff,diff);
           if (complete == -1){
               fprintf(stderr,"write error -1\n");
           }
           // if there are any bytes left we need to process
           diff -= complete;
           if (!diff){
               break;
           }
           else{
               buff += complete;
           }
       }
}
static void receive_callback_output(int type, payload * received_payload, size_t len, int fd){
    if (type){
        printf("Server callback: %s->%s %ld\n", "LWS_CALLBACK_SERVER_RECEIVE",
               &(received_payload->data[LWS_SEND_BUFFER_PRE_PADDING]), len);
    }
    else {
        if (received_payload->op == SEARCH_OP){
            printf("%s\n",
                   &(received_payload->data[LWS_SEND_BUFFER_PRE_PADDING]));

        }
        else {
            char buff[EXAMPLE_RX_BUFFER_BYTES + 1] = {0};
            strcat(buff,(char*) &(received_payload->data[LWS_SEND_BUFFER_PRE_PADDING]));
            strcat(buff,"\n");
            write_complete(fd,buff,strlen(buff));
        }
    }
}

// if it is not one time transfer, we will handle it multiple time instead of onetime
int receive_callback(struct lws *wsi, payload * received_payload, void* in, size_t len, int type, int fd ) {
    if (lws_is_first_fragment(wsi) && lws_is_final_fragment(wsi)){
        memcpy(&(received_payload->data[LWS_SEND_BUFFER_PRE_PADDING]), in, len);
        received_payload->len = len;
        received_payload->data[LWS_SEND_BUFFER_PRE_PADDING + received_payload->len] = '\0';
        receive_callback_output(type, received_payload, len, fd);
        return 1;
    }
    if (lws_is_first_fragment(wsi) && !lws_is_final_fragment(wsi) && type) {
        fprintf(stderr, "Buffer data size is %ld byte and it exceeds max length %d\n",
                EXAMPLE_RX_BUFFER_BYTES + lws_remaining_packet_payload(wsi),EXAMPLE_RX_BUFFER_BYTES);
        return 0;
    }
    return 0;

}

int compare_op(char* src, unsigned char* dst){
    memcpy(src,dst,6);
    src[6] = '\0';
    return strcmp(src,search_syms);
}
int compare_date(char* src, unsigned char* dst){
    memcpy(src,dst,5);
    src[5] = '\0';
    return strcmp(src,"today");
}
// flag 1 is normal case when add to db;
// flag 0 is search today;
char* date_copy(char* original,int flag){
    const char s[3] = " \n";
    char *token;
    /* get the first token */
    token = strtok(original, s);
    int part = 0;
    /* walk through other tokens */
    // Sat Jan  9 03:48:07 2021
    // Sat Jan/9/2021 03:48:07
    char total[64] = {0};
    char time[16];
    while( token != NULL ) {
        if(part == 0 && flag){
            strcat(total,token);
            strcat(total, " ");
        }
        else if (part == 1){

            strcat(total,token);
            strcat(total,"/");
        }
        else if (part == 2){
            strcat(total,token);
            strcat(total,"/");
        }
        else if (part == 3 && flag){
            strcpy(time,token);
        }
        else if (part == 4){
            strcat(total,token);
            if (flag) {
                strcat(total, " ");
            }
        }
        token = strtok(NULL, s);
        part++;
    }
    if (flag) {
        strcat(total, time);
    }
    return strdup(total);

}

void name_copy(char* name, char* message){
    int i = 0;
    for(;*message;message++){
        if(*message == ':'){
            break;
        }else{
            name[i++] = *message;
        }
    }
    name[i] = '\0';
}
void separate_data(unsigned  char* data, char* time, char* real_data){
    size_t time_pointer = 0;
    size_t real_data_pointer = 0;
    bool time_finish = false;
    for(int i = 0 ;data[i]!= '\0';i++){
        if(data[i] == '\n'){
            time_finish = true;
        }
        else{
            if (time_finish){
                real_data[real_data_pointer++] = data[i];
            }
            else{
                time[time_pointer++] = data[i];
            }
        }
    }
    time[time_pointer] = '\0';
    real_data[real_data_pointer] = '\0';
}
void delete_prev_log_file(char* name) {
    bool remove_flag = false;
    if (access(name, F_OK) != 0) {
        return;
    }
    else{
        if (strcmp(name,logs) != 0) {
            remove_flag = true;
        } else {
            struct stat st;
            stat(logs, &st);
            off_t size = st.st_size;
            if (size > one_byte) {
                remove_flag = true;
            }
        }
    }
    if (remove_flag) {
        if (remove(name) != 0) {
            fprintf(stderr, "unable to delete log file in client side %d\n",errno);
        }
    }

}
std::vector<char*>::iterator check_user_name(char* user_name, std::vector<char*>& name_list){
    std::vector<char*>::iterator itr;
    for (itr = name_list.begin(); itr !=name_list.end(); ++itr) {
        // not allow the same id;
        if (strcmp(user_name, *itr) == 0) {
            return itr;
        }
    }
    return itr;
}
struct lws * check_user_wsi(char* user_name, std::map<struct lws *,char*> wsi_map){
    std::map<struct lws *,char*>::iterator itr;
    for (itr = wsi_map.begin(); itr !=wsi_map.end(); ++itr) {

        if (strcmp(user_name, itr->second) == 0) {
            return itr->first;
        }
    }
    return NULL;
}