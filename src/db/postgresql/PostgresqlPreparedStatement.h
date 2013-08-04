/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.
 */
#ifndef POSTGRESQLPREPAREDSTATEMENT_INCLUDED
#define POSTGRESQLPREPAREDSTATEMENT_INCLUDED
#define T PreparedStatementDelegate_T
T PostgresqlPreparedStatement_new(PGconn *db, int maxRows, char *stmt, int paramCount);
void PostgresqlPreparedStatement_free(T *P);
void PostgresqlPreparedStatement_setString(T P, int parameterIndex, const char *x);
void PostgresqlPreparedStatement_setInt(T P, int parameterIndex, int x);
void PostgresqlPreparedStatement_setLLong(T P, int parameterIndex, long long int x);
void PostgresqlPreparedStatement_setDouble(T P, int parameterIndex, double x);
void PostgresqlPreparedStatement_setBlob(T P, int parameterIndex, const void *x, int size);
void PostgresqlPreparedStatement_execute(T P);
ResultSet_T PostgresqlPreparedStatement_executeQuery(T P);
long long int PostgresqlPreparedStatement_rowsChanged(T P);
#undef T
#endif
