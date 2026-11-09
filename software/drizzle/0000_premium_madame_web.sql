CREATE TABLE `limits` (
	`key` text PRIMARY KEY NOT NULL,
	`count` integer NOT NULL
);
--> statement-breakpoint
CREATE TABLE `participants` (
	`room_id` text NOT NULL,
	`client_id` text NOT NULL,
	`seen_at` integer NOT NULL,
	PRIMARY KEY(`room_id`, `client_id`)
);
--> statement-breakpoint
CREATE INDEX `participants_room_seen` ON `participants` (`room_id`,`seen_at`);--> statement-breakpoint
CREATE TABLE `rooms` (
	`id` text PRIMARY KEY NOT NULL,
	`state` text NOT NULL,
	`revision` integer NOT NULL,
	`updated_at` integer NOT NULL
);
