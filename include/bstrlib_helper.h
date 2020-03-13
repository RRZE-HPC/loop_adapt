/*
 * =======================================================================================
 *
 *      Filename:  bstrlib_helper.h
 *
 *      Description:  Additional functions to the bstrlib library (header file)
 *
 *      Version:   5.0
 *      Released:  10.11.2019
 *
 *      Author:   Thomas Gruber (tr), thomas.roehl@googlemail.com
 *      Project:  likwid
 *
 *      Copyright (C) 2019 RRZE, University Erlangen-Nuremberg
 *
 *      This program is free software: you can redistribute it and/or modify it under
 *      the terms of the GNU General Public License as published by the Free Software
 *      Foundation, either version 3 of the License, or (at your option) any later
 *      version.
 *
 *      This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *      WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *      PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along with
 *      this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =======================================================================================
 */

#ifndef BSTRLIB_HELPER_INCLUDE
#define BSTRLIB_HELPER_INCLUDE

#include <bstrlib.h>

#ifdef __cplusplus
extern "C" {
#endif


int bstrListAdd(struct bstrList * sl, bstring str);
int bstrListAddChar(struct bstrList * sl, char* str);

int bstrListDel(struct bstrList * sl, int idx);

bstring bstrListGet(struct bstrList * sl, int idx);

int bstrListContained(struct bstrList * sl, bstring t);

struct bstrList* bstrListUnique(struct bstrList * sl);

void bstrListPrint(struct bstrList * sl);

int btrimbrackets (bstring b);
int btrimsqbrackets (bstring b);
int btrimcurlybrackets (bstring b);

int bisnumber(bstring b);

bstring read_file(char *filename);

int batoi(bstring b);

#ifdef __cplusplus
}
#endif

#endif /* BSTRLIB_HELPER_INCLUDE */
