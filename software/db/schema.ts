import {sqliteTable,text,integer,primaryKey,index} from 'drizzle-orm/sqlite-core';
export const rooms=sqliteTable('rooms',{id:text('id').primaryKey(),state:text('state').notNull(),revision:integer('revision').notNull(),updatedAt:integer('updated_at').notNull()});
export const participants=sqliteTable('participants',{roomId:text('room_id').notNull(),clientId:text('client_id').notNull(),seenAt:integer('seen_at').notNull()},t=>[primaryKey({columns:[t.roomId,t.clientId]}),index('participants_room_seen').on(t.roomId,t.seenAt)]);
export const limits=sqliteTable('limits',{key:text('key').primaryKey(),count:integer('count').notNull()});
