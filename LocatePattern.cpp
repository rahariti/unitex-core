 /*
  * Unitex
  *
  * Copyright (C) 2001-2006 Universit� de Marne-la-Vall�e <unitex@univ-mlv.fr>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 2.1 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  * 
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
  *
  */

//---------------------------------------------------------------------------
#include "LocatePattern.h"

//---------------------------------------------------------------------------



unsigned char* index_controle;
unsigned char** index_code_gramm;
int pattern_compose_courant=0;
struct noeud_code_gramm *racine_code_gramm;
int ESPACE;
int CHAR_BY_CHAR;
struct liste_nombres* tag_token_list=NULL;


/* $CD$ begin */
#ifdef TRE_WCHAR
MasterGF_T* masterGF;
IndexGF_T*  indexGF;
#endif

/*----------------------------------------------------------------------------*/

int locate_pattern(char* text,char* tokens,char* fst2,char* dlf,char* dlc,char* err,
                   char* alphabet,int mode,int output_mode, char* dynamicDir) {
/* $CD$ end   */

FILE* text_file;
FILE* out;
FILE* info;
long int text_size=u_file_size(text)/4;
text_file=fopen(text,"rb");
if (text_file==NULL) {
   fprintf(stderr,"Cannot load %s\n",text);
   return 0;
}
char concord[1000];
char concord_info[1000];

strcpy(concord,dynamicDir);
strcat(concord,"concord.ind");

strcpy(concord_info,dynamicDir);
strcat(concord_info,"concord.n");

out=u_fopen(concord,U_WRITE);
if (out==NULL) {
   fprintf(stderr,"Cannot write %s\n",concord);
   fclose(text_file);
   return 0;
}
info=u_fopen(concord_info,U_WRITE);
if (info==NULL) {
   fprintf(stderr,"Cannot write %s\n",concord_info);
}
switch(output_mode) {
   case IGNORE_TRANSDUCTIONS: u_fprints_char("#I\n",out); break;
   case MERGE_TRANSDUCTIONS: u_fprints_char("#M\n",out); break;
   case REPLACE_TRANSDUCTIONS: u_fprints_char("#R\n",out); break;
}
printf("Loading alphabet...\n");
Alphabet* alph=load_alphabet(alphabet);
if (alph==NULL) {
   fprintf(stderr,"Cannot load alphabet file %s\n",alphabet);
   return 0;
}
struct string_hash* semantic_codes=new_string_hash();
extract_semantic_codes(dlf,semantic_codes);
extract_semantic_codes(dlc,semantic_codes);
printf("Loading fst2...\n");
Automate_fst2* automate=load_fst2(fst2,1);
if (automate==NULL) {
   fprintf(stderr,"Cannot load grammar %s\n",fst2);
   free_alphabet(alph);
   free_string_hash(semantic_codes);
   return 0;
}
/* $CD$ begin */
#ifdef TRE_WCHAR
masterGF = CreateMasterGF( automate , alph );
if (masterGF == NULL) {
   fprintf(stderr,"Cannot compile filter(s)\n");
   free_alphabet(alph);
   free_string_hash(semantic_codes);
   return 0;
   }
#endif
/* $CD$ end   */


printf("Loading token list...\n");
struct string_hash* tok=load_text_tokens_hash(tokens,&SENTENCE_DELIMITER_INDICE,&STOP_MARKER_INDICE);
if (tok==NULL) {
   fprintf(stderr,"Cannot load token list %s\n",tokens);
   free_alphabet(alph);
   free_string_hash(semantic_codes);
   free_fst2(automate);
   return 0;
}


/* $CD$ begin */
#ifdef TRE_WCHAR
indexGF = CreateIndexGF( masterGF, tok );
if (indexGF == NULL) {
   fprintf(stderr,"Cannot optimize filter(s)\n");
   free_alphabet(alph);
   free_string_hash(semantic_codes);
   free_string_hash(tok);
   free_fst2(automate);
   return 0;
   }
#endif
/* $CD$ end   */

extract_semantic_codes_from_tokens(tok,semantic_codes);
index_controle=(unsigned char*)malloc(NUMBER_OF_TEXT_TOKENS*sizeof(unsigned char));
if (index_controle==NULL) {
   fprintf(stderr,"Error: not enough memory\n");
   exit(1);
}
index_code_gramm=(unsigned char**)malloc(NUMBER_OF_TEXT_TOKENS*sizeof(unsigned char*));
if (index_code_gramm==NULL) {
   fprintf(stderr,"Error: not enough memory\n");
   exit(1);
}
for (int i=0;i<NUMBER_OF_TEXT_TOKENS;i++) {
  index_controle[i]=0;
  index_code_gramm[i]=NULL;
}
compute_token_controls(tok,alph,err);
int nombre_patterns=0;
racine_code_gramm=nouveau_noeud_code_gramm();
int existe_etiquette_DIC=0;
int existe_etiquette_CDIC=0;
int existe_etiquette_SDIC=0;
printf("Computing fst2 tags...\n");
numerote_tags(automate,tok,&nombre_patterns,semantic_codes,alph,&existe_etiquette_DIC,
              &existe_etiquette_CDIC,&existe_etiquette_SDIC);

// on calcule l'espace necessaire pour stocker un code grammatical
//
// ???? Verifier que ca marche meme si on augmente le nombre de patterns avec les composes
//
int n_octet_code_gramm=((nombre_patterns+1)/8)+1;
//
// ????
//

// on demarre la numerotation des patterns composes apres celle des patterns
// simples

pattern_compose_courant=nombre_patterns+1;

struct DLC_tree_info DLC_tree;
init_DLC_tree(&DLC_tree,tok->N);

printf("Loading dlf...\n");
load_dic_for_locate(dlf,alph,tok,n_octet_code_gramm,existe_etiquette_DIC,existe_etiquette_CDIC,existe_etiquette_SDIC,&DLC_tree);
printf("Loading dlc...\n");
load_dic_for_locate(dlc,alph,tok,n_octet_code_gramm,existe_etiquette_DIC,existe_etiquette_CDIC,existe_etiquette_SDIC,&DLC_tree);
// we look if the tag tokens like {today,.ADV} verify some patterns

check_patterns_for_tag_tokens(alph,tok,n_octet_code_gramm,&DLC_tree);

printf("Optimizing fst2 tags...\n");
replace_pattern_tags(automate,alph,tok,&DLC_tree);
printf("Optimizing compound word dictionary...\n");
optimize_dlc(&DLC_tree);
free_string_hash(semantic_codes);
init_transduction_variable_index(automate->variables);
printf("Optimizing fst2...\n");
optimize_fst2(automate);
printf("Optimizing patterns...\n");
init_pattern_transitions(tok);
convert_pattern_lists(tok);
printf("Working...\n");
launch_locate(text_file,automate,mode,tok,out,output_mode,text_size,info,&DLC_tree);
free_transduction_variable_index();
fclose(text_file);
if (info!=NULL) u_fclose(info);
u_fclose(out);
printf("Freeing memory...\n");
free_fst2(automate);
free_alphabet(alph);
free_string_hash(tok);
free_liste_nombres(tag_token_list);


/* $CD$ begin */
#ifdef TRE_WCHAR
FreeMasterGF( masterGF, 0 );
FreeIndexGF( indexGF, 0 );
#endif
/* $CD$ end   */


printf("Done.\n");
return 1;
}



void numerote_tags(Automate_fst2* fst2,struct string_hash* tok,int* nombre_patterns,
                   struct string_hash* semantic_codes,Alphabet* alph,
                   int* existe_etiquette_DIC,int* existe_etiquette_CDIC,
                   int* existe_etiquette_SDIC) {
int i,j,k;
unichar tmp[TAILLE_MOT];
unichar flechi[TAILLE_MOT];
unichar canonique[TAILLE_MOT];
unichar pattern[TAILLE_MOT];
Etiquette* etiquette=fst2->etiquette;
unichar t[2];
t[0]=' ';
t[1]='\0';
ESPACE=get_token_number(t,tok);
for (i=0;i<fst2->nombre_etiquettes;i++) {
  if (etiquette[i]->controle&START_VAR_TAG_BIT_MASK) {
     // case of $a(
     etiquette[i]->numero=VAR_START;
     etiquette[i]->controle=(unsigned char)((etiquette[i]->controle-START_VAR_TAG_BIT_MASK)|CONTROL_TAG_BIT_MASK);
  }
  else
  if (etiquette[i]->controle&END_VAR_TAG_BIT_MASK) {
     // case of $a)
     etiquette[i]->numero=VAR_END;
     etiquette[i]->controle=(unsigned char)((etiquette[i]->controle-END_VAR_TAG_BIT_MASK)|CONTROL_TAG_BIT_MASK);
  }
  else
  if (etiquette[i]->controle&POSITIVE_CONTEXT_MASK) {
     // case of $[
     etiquette[i]->numero=POSITIVE_CONTEXT_MARK;
     etiquette[i]->controle=(unsigned char)((etiquette[i]->controle-POSITIVE_CONTEXT_MASK)|CONTROL_TAG_BIT_MASK);
  }
  else
  if (etiquette[i]->controle&NEGATIVE_CONTEXT_MASK) {
     // case of $![
     etiquette[i]->numero=NEGATIVE_CONTEXT_MARK;
     etiquette[i]->controle=(unsigned char)((etiquette[i]->controle-NEGATIVE_CONTEXT_MASK)|CONTROL_TAG_BIT_MASK);
  }
  else
  if (etiquette[i]->controle&CONTEXT_END_MASK) {
     // case of $]
     etiquette[i]->numero=CONTEXT_END_MARK;
     etiquette[i]->controle=(unsigned char)((etiquette[i]->controle-CONTEXT_END_MASK)|CONTROL_TAG_BIT_MASK);
  }
  else
  if (!u_strcmp_char(etiquette[i]->contenu,"#")) {
    if (etiquette[i]->controle&RESPECT_CASE_TAG_BIT_MASK) {
       // on est dans le cas @#: # doit etre considere comme un token normal
       etiquette[i]->numero=get_hash_number(etiquette[i]->contenu,tok);
       index_controle[tok->N]=get_controle(etiquette[i]->contenu,alph,NULL);
       etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|TOKEN_TAG_BIT_MASK);
    }
    else {
       etiquette[i]->numero=DIESE;
       etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|CONTROL_TAG_BIT_MASK);
    }
  }
  else
  if (!u_strcmp_char(etiquette[i]->contenu,"<E>")) {
     etiquette[i]->numero=EPSILON;
     etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|CONTROL_TAG_BIT_MASK);
  }
  else  {
      etiquette[i]->numero=get_token_number(etiquette[i]->contenu,tok);
      if ((etiquette[i]->numero<0) && u_strcmp_char(etiquette[i]->contenu,"<")) {
        if (etiquette[i]->contenu[0]=='<') {
          if (etiquette[i]->contenu[1]=='!') {
            etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|NEGATION_TAG_BIT_MASK);
            j=2;
          } else j=1;
          k=0;
          while (etiquette[i]->contenu[j]!='>')
            tmp[k++]=etiquette[i]->contenu[j++];
          tmp[k]='\0';
          if (!u_strcmp_char(tmp,"MOT")) {
            etiquette[i]->numero=MOT;
    		etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|CONTROL_TAG_BIT_MASK);
          } else if (!u_strcmp_char(tmp,"DIC")) {
            // <DIC> will be computed from 'dlf' and 'dlc'
            // <!DIC> will be computed from the 'err'
            // we do this because part of compound words that are not simple
            // words would recognized by <!DIC>, and we do not want 'aujourd' to
            // appear as an unknown word while it is part of "aujourd'hui"
            if (etiquette[i]->contenu[1]!='!') (*existe_etiquette_DIC)=1;
            etiquette[i]->numero=DIC;
    		etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|CONTROL_TAG_BIT_MASK);
          } else if (!u_strcmp_char(tmp,"CDIC")) {
            (*existe_etiquette_CDIC)=1;
            etiquette[i]->numero=CDIC;
    		etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|CONTROL_TAG_BIT_MASK);
          } else if (!u_strcmp_char(tmp,"SDIC")) {
            (*existe_etiquette_SDIC)=1;
            etiquette[i]->numero=SDIC;
    		etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|CONTROL_TAG_BIT_MASK);
          } else if (!u_strcmp_char(tmp,"MAJ")) {
            etiquette[i]->numero=MAJ;
    		etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|CONTROL_TAG_BIT_MASK);
          } else if (!u_strcmp_char(tmp,"MIN")) {
            etiquette[i]->numero=MIN;
    		etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|CONTROL_TAG_BIT_MASK);
          } else if (!u_strcmp_char(tmp,"PRE")) {
            etiquette[i]->numero=PRE;
    		etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|CONTROL_TAG_BIT_MASK);
          } else if (!u_strcmp_char(tmp,"NB")) {
            etiquette[i]->numero=NB;
    		etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|CONTROL_TAG_BIT_MASK);

            /* $CD$ begin */
          } else if (!u_strcmp_char(tmp,"TOKEN")) {
            etiquette[i]->numero=TOKEN;
    		etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|CONTROL_TAG_BIT_MASK);
            /* $CD$ end   */

          } else {
            // on a une expression entre angles
            // 4 cas possibles
            decouper_entre_angles(tmp,flechi,canonique,pattern,semantic_codes,alph);
            // 1er cas: <V>
            if ((pattern[0]!='\0')&&(flechi[0]=='\0')&&(canonique[0]=='\0')) {
               etiquette[i]->numero=(*nombre_patterns);
    		   etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|GRAMM_CODE_TAG_BIT_MASK);
               inserer_code_gramm(*nombre_patterns,pattern,NULL);
               (*nombre_patterns)++;
            } else
            // 2eme cas: <manger.V>
            if ((flechi[0]=='\0')&&(canonique[0]!='\0')&&(pattern[0]!='\0')) {
               etiquette[i]->numero=(*nombre_patterns);
    		   etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|GRAMM_CODE_TAG_BIT_MASK);
               etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|LEMMA_TAG_BIT_MASK);
               inserer_code_gramm(*nombre_patterns,pattern,canonique);
               (*nombre_patterns)++;
               etiquette[i]->canonique=(unichar*)malloc((u_strlen(canonique)+1)*sizeof(unichar));
               u_strcpy(etiquette[i]->canonique,canonique);
            } else
            // 3eme cas: <mange,manger.V>
            if ((flechi[0]!='\0')&&(canonique[0]!='\0')&&(pattern[0]!='\0')) {
               etiquette[i]->numero=(*nombre_patterns);
    		   etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|GRAMM_CODE_TAG_BIT_MASK);
               etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|LEMMA_TAG_BIT_MASK);
               inserer_code_gramm(*nombre_patterns,pattern,canonique);
               (*nombre_patterns)++;
               etiquette[i]->canonique=(unichar*)malloc((u_strlen(canonique)+1)*sizeof(unichar));
               u_strcpy(etiquette[i]->canonique,canonique);
               etiquette[i]->flechi=(unichar*)malloc((u_strlen(flechi)+1)*sizeof(unichar));
               u_strcpy(etiquette[i]->flechi,flechi);
            } else
            // 4eme cas: <manger>
            if ((flechi[0]=='\0')&&(canonique[0]!='\0')&&(pattern[0]=='\0')) {
               etiquette[i]->numero=LEXICAL_TAG;
               etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|TOKEN_TAG_BIT_MASK);
               etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|LEMMA_TAG_BIT_MASK);
               etiquette[i]->canonique=(unichar*)malloc((u_strlen(canonique)+1)*sizeof(unichar));
               u_strcpy(etiquette[i]->canonique,canonique);
            }// si on n'est dans aucun de ces 4 cas, c'est une erreur
          }
        }
        else {
          // si l'etiquette n'est pas dans les tokens, on l'y rajoute
          // a cause du feature B.C.
          etiquette[i]->numero=get_hash_number(etiquette[i]->contenu,tok);
          index_controle[tok->N]=get_controle(etiquette[i]->contenu,alph,NULL);
          etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|TOKEN_TAG_BIT_MASK);
        }
      } else {
          if (etiquette[i]->numero!=ESPACE) {
            etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|TOKEN_TAG_BIT_MASK);
          }
          else {
            etiquette[i]->numero=SPACE_TAG;
            etiquette[i]->controle=(unsigned char)(etiquette[i]->controle|CONTROL_TAG_BIT_MASK);
          }
        }
    }
}
}



unsigned char get_controle(unichar* s,Alphabet* alph,struct string_hash* err) {
int i;
int tmp;
unsigned char c=0;
if (s[0]=='\0') {
   return 0;
}
if (is_letter(s[0],alph)) {
  c=(unsigned char)(c|MOT_TOKEN_BIT_MASK);
  // if a token is a word, we check if it is in the 'err' word list
  // in order to answer the question <!DIC>
  if (err!=NULL && get_token_number(s,err)!=-1) {
     c=(unsigned char)(c|NOT_DIC_TOKEN_BIT_MASK);
  }
  if (is_upper(s[0],alph)) {
    c=(unsigned char)(c|PRE_TOKEN_BIT_MASK);
    i=0;
    tmp=0;
    while (s[i]!='\0') {
      if (is_lower(s[i],alph)) {
        tmp=1;
        break;
      }
      i++;
    }
    if (!tmp) {
      return (unsigned char)(c|MAJ_TOKEN_BIT_MASK);
    }
  return c;
  }
  else {
    i=0;
    tmp=0;
    while (s[i]!='\0') {
      if (is_upper(s[i],alph)) {
        tmp=1;
        break;
      }
      i++;
    }
    if (!tmp) return (unsigned char)(c|MIN_TOKEN_BIT_MASK);
    return c;
  }
}
else {
   // we check if the token is a tag like {today,.ADV}
   if (s[0]=='{' && u_strcmp_char(s,"{S}") && u_strcmp_char(s,"{STOP}")) {
      // anyway, such a tag is classed as verifying <MOT> and <DIC>
      c=(MOT_TOKEN_BIT_MASK|DIC_TOKEN_BIT_MASK);
      dic_entry* temp=tokenize_tag_token(s);
      if (is_upper(temp->inflected[0],alph)) {
         c=(unsigned char)(c|PRE_TOKEN_BIT_MASK);
         i=0;
         tmp=0;
         while (temp->inflected[i]!='\0') {
            if (is_letter(temp->inflected[i],alph) && is_lower(temp->inflected[i],alph)) {
               tmp=1;
               break;
            }
            i++;
         }
         if (!tmp) {
            c=(unsigned char)(c|MAJ_TOKEN_BIT_MASK);
         }
      }
      else {
         i=0;
         tmp=0;
         while (temp->inflected[i]!='\0') {
            if (is_letter(temp->inflected[i],alph) && is_upper(temp->inflected[i],alph)) {
               tmp=1;
               break;
            }
            i++;
         }
         if (!tmp) {
            c=(unsigned char)(c|MIN_TOKEN_BIT_MASK);
         }
      }
      if (!est_un_mot_simple(temp->inflected,alph)) {
         // if the tag is a compound word, we say that it verifies the <CDIC> pattern
         c=(unsigned char)(c|CDIC_TOKEN_BIT_MASK);
      }
      free_dic_entry(temp);
      return c;
   }
}
return 0;
}


//
// renvoie 1 si s est une categorie grammaticale, 0 sinon
//
int est_categorie_grammaticale(unichar* s,struct string_hash* semantic_codes) {
if (get_token_number(s,semantic_codes)!=-1) {
   return 1;
}
return 0;
}



//
// renvoie 1 si s peut etre un pattern (V:Kms, N+Hum, ...), 0 sinon
//
int est_pattern(unichar* s,struct string_hash* semantic_codes) {
int i;
if ((s==NULL)||(s[0]=='\0')) {
  return 0;
}
i=0;
while ((s[i]!='\0')&&(s[i]!='+')&&(s[i]!='-')&&(s[i]!=':')) {
   i++;
}
// si on a un + un - ou un : on a affaire a un pattern
if (s[i]!='\0') {
   return 1;
}
// sinon, si s est une categorie, on dit que c'est un pattern
if (est_categorie_grammaticale(s,semantic_codes)) {
   return 1;
}
return 0;
}





void decouper_entre_angles(unichar* tmp,unichar* flechi,
                           unichar* canonique,unichar* pattern,
                           struct string_hash* semantic_codes,
                           Alphabet* alph) {
int i,j;
flechi[0]='\0';
canonique[0]='\0';
pattern[0]='\0';
if ((tmp==NULL)||(tmp[0]=='\0')) {
   fprintf(stderr,"The empty pattern <> has been found\n");
   return;
}

i=0;
int k=0;
while ((tmp[i]!=',')&&(tmp[i]!='.')&&(tmp[i]!='\0')) {
   if (tmp[i]=='\\') {i++;}
   flechi[k++]=tmp[i++];
}
flechi[k]='\0';
// 1er cas: <manger> -> on a juste une forme canonique
if (tmp[i]=='\0') {
   // we must test on tmp and NOT on flechi, because of patterns like
   // "<A+faux\-ami>". In fact, tmp contains "A+faux\-ami" and flechi contains
   // "A+faux-ami"
   // So, if we consider flechi instead of tmp, the minus will be taken as
   // a negation and not as a part of the code "faux-ami", and then, no
   // difference will be made between "<A+faux\-ami>" and "<A+faux-ami>".
   if (est_pattern(tmp,semantic_codes)) {
      pattern[0]='\0';
      u_strcpy(pattern,tmp);
      flechi[0]='\0';
      canonique[0]='\0';
      return;
   }
   else {
      u_strcpy(canonique,flechi);
      flechi[0]='\0';
      return;
   }
}
// on est ou dans le cas <manger.V> ou dans le cas <.V>
if (tmp[i]=='.') {
   u_strcpy(canonique,flechi);
   flechi[0]='\0';
   i++;
   j=0;
   while (tmp[i]!='\0') {
      pattern[j++]=tmp[i++];
   }
   pattern[j]='\0';
   return;
}
// on est dans le cas <mange,manger.V>
if (flechi[0]=='\0') {
   fprintf(stderr,"Invalid pattern has been found\n");
   return;
}
i++;
j=0;
while ((tmp[i]!='.')&&(tmp[i]!='\0')) {
   canonique[j++]=tmp[i++];
}
canonique[j]='\0';
if (j==0) {
   fprintf(stderr,"Invalid pattern has been found\n");
   return;
}
if (tmp[i]=='\0') {
   fprintf(stderr,"Invalid pattern has been found\n");
   return;
}
i++;
j=0;
while ((tmp[i]!='.')&&(tmp[i]!='\0')) {
   pattern[j++]=tmp[i++];
}
pattern[j]='\0';
}



void compute_token_controls(struct string_hash* tok,Alphabet* alph,char* err) {
struct string_hash* ERR=load_word_list(err);
for (int i=0;i<tok->N;i++) {
  index_controle[i]=get_controle(tok->tab[i],alph,ERR);
}
free_string_hash(ERR);
}

