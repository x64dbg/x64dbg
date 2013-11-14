#include "BeaHighlight.h"

BeaHighlight::BeaHighlight()
{
}

SEGMENTREG BeaHighlight::ConvertBeaSeg(int beaSeg)
{
    switch(beaSeg)
    {
    case ESReg:
        return SEG_ES;
        break;
    case DSReg:
        return SEG_DS;
        break;
    case FSReg:
        return SEG_FS;
        break;
    case GSReg:
        return SEG_GS;
        break;
    case CSReg:
        return SEG_CS;
        break;
    case SSReg:
        return SEG_SS;
        break;
    }
    return SEG_DEFAULT;
}

bool BeaHighlight::PrintArgument(QList<CustomRichText_t>* richText, const ARGTYPE* Argument, const INSTRTYPE* Instruction, bool* had_arg)
{
    CustomRichText_t argument;
    argument.flags=FlagNone;
    int argtype=Argument->ArgType;
    int brtype=Instruction->BranchType;
    QString argmnemonic=QString(Argument->ArgMnemonic);
    if(argtype!=NO_ARGUMENT && *Argument->ArgMnemonic)
    {
        if(*had_arg) //add a comma
        {
            CustomRichText_t comma;
            comma.text=",";
            comma.flags=FlagNone;
            richText->push_back(comma);
        }

        if(argtype&MEMORY_TYPE) //mov [],a || mov a,[]
        {
            char segment[3]="";
            int segmentReg=Argument->SegmentReg;
            switch(segmentReg)
            {
            case ESReg:
                strcpy(segment, "es");
                break;
            case DSReg:
                strcpy(segment, "ds");
                break;
            case FSReg:
                strcpy(segment, "fs");
                break;
            case GSReg:
                strcpy(segment, "gs");
                break;
            case CSReg:
                strcpy(segment, "cs");
                break;
            case SSReg:
                strcpy(segment, "ss");
                break;
            }
            int basereg=Argument->Memory.BaseRegister;
            if(basereg&REG4 || basereg&REG5) //esp || ebp
            {
                argument.textBackground=QColor(0,255,255);
                argument.flags=FlagBackground;
                //Highlight ESP || EBP memory move
            }
            else
            {
                argument.textColor=QColor(0,0,128);
                argument.flags=FlagColor;
            }

            //labels
            uint_t label_addr=Argument->Memory.Displacement;
            char label_text[MAX_LABEL_SIZE]="";
            if(DbgGetLabelAt(label_addr, ConvertBeaSeg(segmentReg), label_text))
            {
                QString label_addr_text=QString("%1").arg(label_addr, 0, 16, QChar('0')).toUpper();
                if(argmnemonic.indexOf(label_addr_text)!=-1)
                {
                    argmnemonic.replace(label_addr_text, "<"+QString(label_text)+">");
                }
            }

            switch(Argument->ArgSize)
            {
            case 8:
                argument.text.sprintf("byte ptr %s:[%s]", segment, argmnemonic.toUtf8().constData());
                break;
            case 16:
                argument.text.sprintf("word ptr %s:[%s]", segment, argmnemonic.toUtf8().constData());
                break;
            case 32:
                argument.text.sprintf("dword ptr %s:[%s]", segment, argmnemonic.toUtf8().constData());
                break;
            case 64:
                argument.text.sprintf("qword ptr %s:[%s]", segment, argmnemonic.toUtf8().constData());
                break;
            }
        }
        else
        {
            //labels
            uint_t label_addr=Instruction->Immediat;
            if(!label_addr)
                label_addr=Instruction->AddrValue;
            char label_text[MAX_LABEL_SIZE]="";
            char module_text[33]="";
            bool hasLabel=DbgGetLabelAt(label_addr, SEG_DEFAULT, label_text);
            bool hasModule=DbgGetModuleAt(label_addr, module_text);
            QString label_addr_text=QString("%1").arg(label_addr, 0, 16, QChar('0')).toUpper();;
            QString newText;
            if(hasLabel && hasModule) //<module.label>
            {
                newText="<"+QString(module_text)+"."+QString(label_text)+">";
                if(argmnemonic.indexOf(label_addr_text)!=-1)
                {
                    argument.flags=FlagBackground;
                    argument.textBackground=QColor(255,255,0);
                    argmnemonic.replace(label_addr_text, newText);
                }
            }
            else if(hasModule) //module.%llX
            {
                newText=QString(module_text)+"."+QString(label_addr_text);
                if(argmnemonic.indexOf(label_addr_text)!=-1)
                {
                    argument.flags=FlagBackground;
                    argument.textBackground=QColor(255,255,0);
                    argmnemonic.replace(label_addr_text, newText);
                }
            }
            else if(hasLabel) //<label>
            {
                newText="<"+QString(label_text)+">";
                if(argmnemonic.indexOf(label_addr_text)!=-1)
                {
                    argmnemonic.replace(label_addr_text, newText);
                }
            }

            //jumps
            char shortjmp[100]="\0";
            bool has_shortjmp=false;
            if(brtype && brtype!=RetType && !(argtype&REGISTER_TYPE))
            {
                argument.flags=FlagBackground;
                argument.textBackground=QColor(255,255,0);
                unsigned char* opc=(unsigned char*)&Instruction->Opcode;

                if(*opc==0xEB || Instruction->Opcode<0x80)
                {
                    strcpy(shortjmp+1, "short ");
                    has_shortjmp=true;
                    //short jumps
                }
                //Highlight (un)condentional jumps && calls
            }
            argument.text.sprintf("%s%s", shortjmp+has_shortjmp, argmnemonic.toUtf8().constData());
        }
        *had_arg=true;
        richText->push_back(argument);
    }
    else
        return false;
    return true;
}

void BeaHighlight::PrintBaseInstruction(QList<CustomRichText_t>* richText, const DISASM* MyDisasm)
{
    CustomRichText_t mnemonic;
    char mnemonicText[16];
    strcpy(mnemonicText, MyDisasm->Instruction.Mnemonic);
    mnemonicText[strlen(mnemonicText)-1]=0; //remove space
    mnemonic.text=mnemonicText;
    int brtype=MyDisasm->Instruction.BranchType;
    if(brtype) //jump
    {
        if(brtype==RetType || brtype==CallType)
        {
            mnemonic.flags=FlagBackground;
            mnemonic.textBackground=QColor(0,255,255);
            //calls && rets
        }
        else if(brtype==JmpType)
        {
            mnemonic.flags=FlagBackground;
            mnemonic.textBackground=QColor(255,255,0);
            //uncond jumps
        }
        else
        {
            mnemonic.flags=FlagAll;
            mnemonic.textBackground=QColor(255,255,0);
            mnemonic.textColor=QColor(255,0,0);
            //cond jumps
        }
    }
    else if(!_stricmp(mnemonicText, "push") || !_stricmp(mnemonicText, "pop"))
    {
        mnemonic.flags=FlagColor;
        mnemonic.textColor=QColor(0,0,255);
        //push/pop
    }
    else if(!_stricmp(mnemonicText, "nop"))
    {
        mnemonic.flags=FlagColor;
        mnemonic.textColor=QColor(128,128,128);
        //nop
    }
    else
        mnemonic.flags=FlagNone;
    richText->push_back(mnemonic);
}

void BeaHighlight::PrintRtfInstruction(QList<CustomRichText_t>* richText, const DISASM* MyDisasm)
{
    CustomRichText_t space;
    space.text=" ";
    space.flags=FlagNone;

    CustomRichText_t prefix;
    int prefixCount=0;
    if(MyDisasm->Prefix.LockPrefix)
    {
        prefix.text+="lock ";
        prefixCount++;
    }
    if(MyDisasm->Prefix.RepPrefix)
    {
        prefix.text+="rep ";
        prefixCount++;
    }
    if(MyDisasm->Prefix.RepnePrefix)
    {
        prefix.text+="repne ";
        prefixCount++;
    }
    if(prefixCount)
    {
        prefix.flags=FlagNone; //no colors
        richText->push_back(prefix);
    }
    PrintBaseInstruction(richText, MyDisasm);
    richText->push_back(space);
    bool had_arg=false;
    PrintArgument(richText, &MyDisasm->Argument1, &MyDisasm->Instruction, &had_arg);
    PrintArgument(richText, &MyDisasm->Argument2, &MyDisasm->Instruction, &had_arg);
    PrintArgument(richText, &MyDisasm->Argument3, &MyDisasm->Instruction, &had_arg);
}
