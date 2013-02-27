enum im_message_type {
	REGISTRATION_MESSAGE,
	DEREGISTRATION_MESSAGE,
	INSTANT_MESSAGE
};

struct im_message {
	im_message_type type;
	char to[256];
	char from[256];
	char message[1024];
};

