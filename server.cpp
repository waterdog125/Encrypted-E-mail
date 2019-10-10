#include <iostream>
#include <stdio.h>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <stdlib.h>
using namespace std;

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i=0; i<argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    return 0;
}
    
static int callback2(void *outputPtr, int argc, char **argv, char **azColName){
    int i;
    vector<string> *list = reinterpret_cast<vector<string>*>(outputPtr);
    list->push_back(argv[1]);
    return 0;
}
static int callback3(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i=0; i<argc; i++){
        if (i==2){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        }
    }
    return 0;
}

static int readMessage(void *NotUsed, int argc, char **argv, char **azColName){ //for reading and decrypting
    int i;
    string username1;
    cout << "Ensure your private key file is in this directory at this time (format: usernameprivate.pem), enter username when you are ready." << endl;
    getline(cin, username1);
    for(i=0; i<argc; i++){
        while(i < 3){
            printf("%s = %s ", azColName[i], argv[i] ? argv[i] : "NULL");
            i++;
        }
        if(i == 3){
            system(("openssl rsautl -decrypt -inkey "+username1+"private.pem -in "+argv[2]+"encrypted.txt -out decrypted.txt").c_str());
            cout<<endl;
            system(("cat decrypted.txt"));
        }
    }
    cout << endl;
    return 0;
}

static int printOutMessage(void *NotUsed, int argc, char **argv, char **azColName){ //for deleting
    int i;
    for(i=0; i<argc; i++){
        printf("%s = %s ", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    cout << endl;
    return 0;
}

string GetStdoutFromCommand(string cmd) {

    string data;
    FILE * stream;
    const int max_buffer = 256;
    char buffer[max_buffer];
    cmd.append(" 2>&1");

    stream = popen(cmd.c_str(), "r");
    if (stream) {
        while (!feof(stream))
            if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
                pclose(stream);
    }
    return data;
}
int main() {
    sqlite3* serverProgram;
    char *zErrMsg = 0;
    string entry1;
    string entry2;
    string username;
    string password;
    int rc;
    
    
    rc = sqlite3_open("mailSQL.db", &serverProgram);
    
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(serverProgram));
        sqlite3_close(serverProgram);
        return 1;
    }
    
    //test 1- Make tables
    string signinString = "CREATE TABLE sign_in(Username TEXT, Password TEXT);";
    rc = sqlite3_exec(serverProgram, signinString.c_str(), callback,0,&zErrMsg);
    cout << "User table was successfully made\n";
    
    string emailString = "CREATE TABLE emails(Sender TEXT, Receiver TEXT, Subject TEXT, Message TEXT);";
    rc = sqlite3_exec(serverProgram, emailString.c_str(), callback,0,&zErrMsg);
    cout << "Email table was successfully made\n";
    
    
    //test 2- Make user
    cout << "Enter 'sign up' to make an account, enter 'sign in' to sign in, enter 'delete' to delete user, or enter 'quit' to exit the program" << endl;
    cout << "Please enter a command: ";
    getline(cin, entry1);
    while(entry1 != "sign up" && entry1 != "sign in" && entry1 != "delete" && entry1 != "quit"){
        cout << "Please enter a valid command: ";
        getline(cin, entry1);
    }
    while(entry1 != "quit"){
    //test 3- sign up
    if(entry1 == "sign up"){
        cout <<"Enter a username: ";
        getline(cin,username);
        string checkForUser = "SELECT * FROM sign_in WHERE Username='" + username + "' ";
        vector<string> userCheck;
        rc = sqlite3_exec(serverProgram, checkForUser.c_str(),callback2,&userCheck,&zErrMsg);
        cout << "after grabbing info" << endl;
       if(!userCheck.empty()){
                cout << "Username: " + username + " is already taken." <<endl;
            }else{
             cout <<"Enter a password: ";
             getline(cin,password);
             cout <<endl;
            string sql2 = "INSERT into sign_in(Username,Password) values (\""+ username + "\",\"" +password+ "\");";
            rc = sqlite3_exec(serverProgram, sql2.c_str(),NULL,0,&zErrMsg);
            cout <<"username: " + username + " with password: " + password + " was successfully created" << endl;
            system(("openssl genrsa -out "+username+"private.pem").c_str());
            system(("cat "+username+"private.pem").c_str());
            system(("openssl rsa -in "+username+"private.pem -outform PEM -pubout -out "+username+"public.pem").c_str());
            cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl 
            << "IMPORTANT!!! Please make sure to copy your private key somewhere safe, and move/delete your private key file generated."<<endl;
            }
    }
    
    //test 4- sign in        
    else if(entry1 == "sign in"){
        cout <<"Enter your username: ";
        getline(cin,username);
        cout <<"Enter your password: ";
        getline(cin,password);
        cout <<endl;
       
        string sql3 = "SELECT * FROM sign_in WHERE Username='" + username + "' and Password='"+ password + "'";
        vector<string> results;
        
        rc = sqlite3_exec(serverProgram, sql3.c_str(), callback2, &results, &zErrMsg);
        if (!results.empty()){
        cout << "Welcome user: " + username << endl;
        cout << "Enter 'send' to send a message, enter 'read' to read your email, enter 'delete' to delete a email. Enter 'quit' to quit" <<endl;
        cout << "Please enter a command: ";
        getline(cin, entry2);
        while(entry2 != "send" && entry2 != "read" && entry2 != "delete" && entry2 != "quit"){
            cout << "Please enter a valid command: ";
            getline(cin, entry2);
        }
        
        while(entry2 != "quit"){
    
        //test 6- send a message
        if(entry2 == "send"){
        string userToEmail;
        string subject;
        string message;
        sqlite3_exec(serverProgram, "SELECT Username FROM sign_in",callback,0,&zErrMsg);
        cout << "Please enter the username of the user you wish to email: ";
        getline(cin, userToEmail);
        string PubKey = "SELECT Public_Key FROM sign_in WHERE Username IN SELECT= '" +userToEmail+"'"" FROM emails";
        string recipientPubKey;
        sqlite3_exec(serverProgram , PubKey.c_str(), callback2 , &recipientPubKey , &zErrMsg);//attempt
        cout << "Please enter the email subject (no symbols/spaces): ";
        getline(cin, subject);
        cout << "Please enter the message: ";
        getline(cin, message);
        system(("echo "+message+"> emailplain.txt").c_str());
        system(("openssl rsautl -encrypt -inkey "+ userToEmail+"public.pem -pubin -in emailplain.txt -out "+subject+"encrypted.txt").c_str());
        string encrypted = GetStdoutFromCommand("cat "+subject+"encrypted.txt");
        string insertEmail = "INSERT into emails(Sender, Receiver, Subject, Message) values (\""+username+"\",\""+userToEmail+"\",\""+subject+"\",\""+encrypted+"\");";
        rc = sqlite3_exec(serverProgram, insertEmail.c_str(),NULL,0,&zErrMsg);
        cout << "message to " + userToEmail + " was sent." << endl;
        
        //test 7- read a message    
        }else if(entry2 == "read"){
            string getMail = "SELECT * FROM emails WHERE Receiver= '" +username+"'";
            sqlite3_exec(serverProgram, getMail.c_str(),callback3,0,&zErrMsg);
             cout << "Please enter the subject of the email you would like to read: ";
             string subToRead;
             getline(cin, subToRead);
             string getSub = "SELECT * FROM emails WHERE Subject= '"+subToRead+"'";
            sqlite3_exec(serverProgram,getSub.c_str() ,readMessage,0,&zErrMsg);
        
        //test 8- delete a message    
        }else if(entry2 == "delete"){
        string toDeleteSender; 
        string subject;
        string toDeleteReciever;
         string getMail = "SELECT * FROM emails WHERE Receiver= '" +username+"'";
        sqlite3_exec(serverProgram,getMail.c_str() ,printOutMessage,0,&zErrMsg);
        cout << "enter the subject of the email you wish to delete: ";    
        getline(cin, subject);    
        cout << "enter the sender of the email you wish to delete: ";
        getline(cin, toDeleteSender);
        cout << "enter your username: ";
        getline(cin, toDeleteReciever);
        string toDelete = "DELETE FROM emails WHERE Receiver= '"+toDeleteReciever+"' and Subject= '"+ subject + "' and Sender= '" +toDeleteSender+"'";  
        vector<string> canDelete;
        rc = sqlite3_exec(serverProgram, toDelete.c_str(), callback2, &canDelete, &zErrMsg);
        
        if (canDelete.empty()){
        cout << "Email with subject: " + subject + " sent from " + toDeleteSender +" has successfully been deleted.\n" << endl;   
      }else {
        cout << "Invalid subject, sender, or reciever. Unable to delete email." << endl;
      }
        }
      else {
        cout << "Invalid sign in. Username: " + username + " with Password: " + password + " is not in our database." << endl;
        }
        cout << "Enter 'send' to send an email, enter 'read' to read your emails, enter 'delete' to delete an email. Enter 'quit' to quit" <<endl;
        cout << "Please enter a command: ";
        getline(cin, entry2);
        while(entry2 != "send" && entry2 != "read" && entry2 != "delete" &&entry2 != "quit"){
            cout << "Please enter a valid command: ";
            getline(cin, entry2);
        }
    }
    
    //test 5- delete user    
    }else if(entry1 == "delete"){
        cout << "enter a username: ";
        getline(cin, username);
        cout << "enter a password: ";
        getline(cin, password);
        
        string sql3 = "SELECT * FROM sign_in WHERE Username='" + username + "' and Password='"+ password + "'";
        vector<string> results;
        rc = sqlite3_exec(serverProgram, sql3.c_str(), callback2, &results, &zErrMsg);
        
        if (!results.empty()){
        string sql4 = "DELETE FROM sign_in WHERE Username = '" + username + "'";
        vector<string> results2;
        rc = sqlite3_exec(serverProgram, sql4.c_str(), callback2, &results2, &zErrMsg);
        cout << "User " + username + " has successfully been deleted.\n" << endl;   
      }else {
        cout << "Invalid user credentials, unable to delete user " + username << endl;
      }
    }
        
  }
    cout << "Enter 'sign up' to make an account, enter 'sign in' to sign in, enter 'delete' to delete user, or enter 'quit' to exit the program" << endl;
    cout << "Please enter a command: ";
    getline(cin, entry1);
    while(entry1 != "sign up" && entry1 != "sign in" && entry1 != "delete" && entry1 != "quit"){
        cout << "Please enter a valid command: ";
        getline(cin, entry1);
    }
    }//bracket
    string save = ".save mailSQL.db";
    sqlite3_exec(serverProgram, save.c_str(), callback, 0, &zErrMsg);
    sqlite3_close(serverProgram);
    system(("rm emailplain.txt decrypted.txt"));
    cout << "You are logged out. Goodbye.\n";
    return 0;
}