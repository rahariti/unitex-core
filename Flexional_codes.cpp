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
#include "Grammatical_codes.h"
#include "Flexional_codes.h"
#include "Error.h"
//---------------------------------------------------------------------------



Code_flexion nouveau_code_flexion() {
Code_flexion c;
c=(Code_flexion)malloc(sizeof(struct code_flexion));
c->s[0]='\0';
return c;
}



int ajouter_code(Code_flexion c,unichar* s) {
if ((u_strlen(c->s)+u_strlen(s)) > MAX_INFLECTIONAL_CODES_LENGTH) {
   error("Inflectional code too long\n");
   return 0;
}
if (s[0]!=':') {
   u_strcat_char(c->s,":");
}
u_strcat(c->s,s);
return 0;
}



Code_flexion calculer_code_flexion(unichar* t2[]) {
Code_flexion c;
int i;
c=nouveau_code_flexion();
for (i=0;i<MAX_INFLECTIONAL_CODES_LENGTH;i++) {
  if (t2[i]!=NULL) {
     ajouter_code(c,t2[i]);
  }
}
return c;
}


Code_flexion calculer_code_flexion(struct dela_entry* entry) {
Code_flexion c;
int i;
c=nouveau_code_flexion();
for (i=0;i<entry->n_inflectional_codes;i++) {
  ajouter_code(c,entry->inflectional_codes[i]);
}
return c;
}


void ajouter_a_liste_code_flexion(struct noeud_code_gramm* n,Code_flexion c,
                                  int numero_pattern,struct list_ustring* f,
                                  unichar* canonique) {
struct liste_code_flexion* l;
l=(struct liste_code_flexion*)malloc(sizeof(struct liste_code_flexion));
l->numero_pattern=numero_pattern;
l->code=NULL;
if (c==NULL) {
   if (l->code!=NULL) {
      free(l->code);
      l->code=NULL;
   }
} else {
   l->code=nouveau_code_flexion();
   u_strcpy((l->code)->s,c->s);
}
if (f==NULL) {
   l->f=NULL;
}
else {
   l->f=clone(f);
}
if (canonique==NULL || !u_strcmp_char(canonique,"")) {
   l->canonique=NULL;
}
else {
   l->canonique=u_strdup(canonique);
}
l->suivant=n->liste;
n->liste=l;
}
