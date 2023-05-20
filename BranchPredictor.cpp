#include<iostream>
#include<fstream>
#include<sstream>
#include "BranchPredictor.hpp"
using namespace std;
int convert_to_decimal(string s){
    int l = s.size() ;
    int ans=0 ;
    for (int i =0 ;  i< l ; i++ ) {
        ans=((ans*16)%(1<<14)) ;
        if (s[i]== 'a') ans+=10 ;
        else if (s[i]== 'a') ans+=10 ;
        else if (s[i]== 'b') ans+=11 ;
        else if (s[i]== 'c') ans+=12 ;
        else if (s[i]== 'd') ans+=13 ;
        else if (s[i]== 'f') ans+=14 ;
        else if (s[i]== 'e') ans+=15 ;
        else ans+=(s[i]-'0') ;
    }
    return ans ;
}
int bool_to_int(bool b){
    if (b) return 1 ;
    else return 0 ;
}
bool char_to_bool(char i){
    if (i=='1') return true ;
    else return false ;
}
int main(){
    for (int ii=0 ; ii<4 ; ii++){
    string myText;
    BHRBranchPredictor predictor1(ii) ;
    SaturatingBranchPredictor predictor2(ii) ;
    SaturatingBHRBranchPredictor predictor3(ii ,1<<14) ;
    ifstream MyReadFile("branch_trace.txt");

    int correct1=0 , wrong1 =0 , correct2=0 , wrong2=0 , correct3=0 ,wrong3=0 ;
    int fp1 =0 , fn1 =0 , tp1 =0 , tn1=0 ;
    int fp2 =0 , fn2 =0 , tp2 =0 , tn2=0 ;
    int fp3 =0 , fn3 =0 , tp3 =0 , tn3=0 ;
    while (getline (MyReadFile, myText)) {
        int pc = convert_to_decimal(myText.substr(0,8)) ;
        if(bool_to_int(predictor1.predict(pc))== (myText[9]-'0')) {
            correct1 ++ ;
            if (bool_to_int(predictor1.predict(pc))) tp1 ++ ;
            else tn1++ ;}
            
        else {
            wrong1 ++ ;
            if (bool_to_int(predictor1.predict(pc))) fp1 ++ ;
            else fn1 ++ ;
        };
        predictor1.update(pc,char_to_bool(myText[9])) ;
        if(bool_to_int(predictor2.predict(pc))== (myText[9]-'0')) {
            correct2 ++ ;
            if (bool_to_int(predictor2.predict(pc))) tp2 ++ ;
            else tn2++ ;}
            
        else {
            wrong2 ++ ;
            if (bool_to_int(predictor2.predict(pc))) fp2 ++ ;
            else fn2 ++ ;
        } ;
        predictor2.update(pc,char_to_bool(myText[9])) ;
        if(bool_to_int(predictor3.predict(pc))== (myText[9]-'0')) {
            correct3 ++ ;
            if (bool_to_int(predictor3.predict(pc))) tp3 ++ ;
            else tn3++ ;}
            
        else {
            wrong3 ++ ;
            if (bool_to_int(predictor3.predict(pc))) fp3 ++ ;
            else fn3 ++ ;
        };
        predictor3.update(pc,char_to_bool(myText[9])) ;
    }
    float total1 = (correct1+wrong1) ;
    float total2 = (correct2+wrong2) ;
    float total3 = (correct3+wrong3) ;
    cout<<"--------------------------------------------------------------"<<endl ;
    cout<<"For initial value of "<<ii<<" "<<endl ;
;   cout<<"Data for BHR predictor"<<endl  ;
    cout<<"Total data count : "<<total1<<endl ;
    cout<<"True positives : "<<tp1<<endl ;
    cout<<"False positives : "<<(fp1)<<endl ;
    cout<<"True negatives : "<<(tn1)<<endl ;
    cout<<"False negatives : "<<(fn1)<<endl ;
    cout<<"Fraction of correct predictions : ";
    cout<<((float)(correct1)/total1)<<endl<<endl; 
    cout<<"Data for 2-Bit Saturating counters predictor"<<endl   ;
    cout<<"Total data count : "<<total2<<endl ;
    cout<<"True positives : "<<(tp2)<<endl ;
    cout<<"False positives : "<<(fp2)<<endl ;
    cout<<"True negatives : "<<(tn2)<<endl ;
    cout<<"False negatives : "<<(fn2)<<endl ;
    cout<<"correct predictions : " ;
    cout<<((float)(correct2)/total2)<<endl<<endl; 
    cout<<"Data for Combination predictor "<<endl ;
    cout<<"Total data count : "<<total3<<endl ;
    cout<<"True positives : "<<(tp3)<<endl ;
    cout<<"False positives : "<<(fp3)<<endl ;
    cout<<"True negatives : "<<(tn3)<<endl ;
    cout<<"False negatives : "<<(fn3)<<endl ;
    cout<<"Fraction of correct predictions : " ;
    cout<<((float)(correct3)/total3)<<endl<<endl; 

}}