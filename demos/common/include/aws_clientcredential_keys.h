/*
 * PEM-encoded client certificate.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----"
 * "...base64 data..."
 * "-----END CERTIFICATE-----";
 */
#ifndef USE_ESEYE
static const char clientcredentialCLIENT_CERTIFICATE_PEM[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDWjCCAkKgAwIBAgIVAK8pJmPtloC8EEM6PIu+QKe68z/NMA0GCSqGSIb3DQEB\n"
"CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\n"
"IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0xODA1MzExMDQ5\n"
"NDBaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\n"
"dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCmR3UaQIHr9/m86Jsg\n"
"gjV65BBqgiG83759pi7kZ3ZcRUu5SiT/efroDlnrovEjovCpI0Er5oaL9msbLlXz\n"
"y11Zw9Fif5cuSzB+ZwDQwgOJMGAdamEalfEyWHNpb/8ZrqoSZZrPjPPfoEd34xPu\n"
"Xar9CNDfUwYnpgw1AH2V1e3NuQs3AQNYy1kpsy4Xcuq7Lo3YN15vnAhn4o70JRNO\n"
"y19adBlVKIgAplqp0jZt10lKf0Z2SRDuNoaTR/JANhZo1Gbt2HxDv4YeYq9o7EUN\n"
"vlYOxRlYaTy+yZjnKjV6gTmN3IBKYgjgEu3koYOq1oQ7m+fEYYi81x5tkFW9FsMD\n"
"0KK9AgMBAAGjYDBeMB8GA1UdIwQYMBaAFMBSrQ6DNmTZzTLV7yTNS3SV0ekEMB0G\n"
"A1UdDgQWBBQG1EgekJq247YzRwlPv7qFSVRMkDAMBgNVHRMBAf8EAjAAMA4GA1Ud\n"
"DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAGyV6E7z0k3lP1dkDdu8Zca9I\n"
"UZ+DijVjczOpGVbiNXmSGuLbMNK+hsITAbgtE3yvnH8Nm5y6Eby4reNE53859VOx\n"
"WrWkbM0ugYYcIhZqFZOEHvUwChne16doN3/GwEYAfSCGqNYQb+jv0W13ysIjDghU\n"
"teDtQMZDu8EH51SJ8KlR9Bn1LyKA1VF73tLPULtuN49fm5f8Em9/FBxZuMtAWdNG\n"
"Bz66ywYTQmnFHp75aBwteBn7JKFlC1/nBmg/piD4TJN4MnmOQunahuHyTZJMC47J\n"
"XQsqZQa/NQoFbCtVgkKmxoaXJPw9pjYvHSKZIQra0hviedF85oo6dxdfDKSmAg==\n"
"-----END CERTIFICATE-----";

/*
 * PEM-encoded client private key.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN RSA PRIVATE KEY-----"
 * "...base64 data..."
 * "-----END RSA PRIVATE KEY-----";
 */
static const char clientcredentialCLIENT_PRIVATE_KEY_PEM[] =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpgIBAAKCAQEApkd1GkCB6/f5vOibIII1euQQaoIhvN++faYu5Gd2XEVLuUok\n"
"/3n66A5Z66LxI6LwqSNBK+aGi/ZrGy5V88tdWcPRYn+XLkswfmcA0MIDiTBgHWph\n"
"GpXxMlhzaW//Ga6qEmWaz4zz36BHd+MT7l2q/QjQ31MGJ6YMNQB9ldXtzbkLNwED\n"
"WMtZKbMuF3Lquy6N2Ddeb5wIZ+KO9CUTTstfWnQZVSiIAKZaqdI2bddJSn9GdkkQ\n"
"7jaGk0fyQDYWaNRm7dh8Q7+GHmKvaOxFDb5WDsUZWGk8vsmY5yo1eoE5jdyASmII\n"
"4BLt5KGDqtaEO5vnxGGIvNcebZBVvRbDA9CivQIDAQABAoIBAQCPwQmBl7F7Ixja\n"
"9DJhKZE43IFAw56NXtaeZJT3zGbsoNA1sd9Iq9l61CVzbZySVaVAZQVMAfHigTjZ\n"
"9/ZBXlknaP7V7D70u/aB4WU4FfPeoF8IL1ciF+29u/CTgEsIlhPc8dCkjVStyjDf\n"
"egdaNcrFFeEWof2ZO5okHHd2mcwM5P/43NHc9G7IPbNC4Ymkko4DKQ68jtNN7O/7\n"
"EjVrpG6vxM/NueQb2iPGGbigMM5bHcYWG7KCVaffF4ANOlZfxdqsi7IIbtvA6+wR\n"
"QTYby+akX5M6Nv+dqI9LfQLpFvmHDy9THH7Hj75jDA3cFuawuoOe++M3TjfEGnLl\n"
"QsUBSx7hAoGBANJI1BoG99g4btDW7f56aKyH3/zatsqadwxJ4CHdKQonMdMyoUw3\n"
"5/5HghYFTTQQbt4tdCSu9iZ2lr2d0z/Qmn132Lbo7AcnxsLsK17QH9fUxBlxVXOL\n"
"EO5tyfeStAPJjHvpLyb81dMBSzxKWLi1mCFfF7961BkdB8MCob9pLE7JAoGBAMpt\n"
"jhwC0GacjCqJVh2o4JSU37vQH27Ele7JeLc0Rt5D5JJdT+rQSJACMaAElst5wwyh\n"
"1VcajtWFKWUoOWO5+NjWIeYzstLWNKHFyQ7GIjwGGOA7CNfu4DgNtFfGzP6waltY\n"
"hR6j0EPTC/VaEB/Tq0v/kP42AmR0P9VuxfP1AapVAoGBAMlnKYZYIC8+NZzeDgoK\n"
"0ZBVfwldrW81LEpfw0SL3w+zZHxEZMpsTU10zborj7sK9jTj0faEgItsT4oCzF61\n"
"fBIppC3jvpRH2427xvpDWgxM4xj/PHmyux+xlZwCLVnnQx82wouT43P6LEXtazZV\n"
"7wQfYM5rZRM8g7+aGLMMl3xJAoGBAL+9qBc2PyqC7G26KrzrYta6cXZX+/4S7aYC\n"
"Znq6ZMpR6ucMxg84fRMTUOgukJtp2WxUulzIRjVP7dbok/u3g1P4+KExpRf6WF5H\n"
"l1uMJ49NgzCaGpVWqkHOEV33a+NvPT8LCQTty+8CsFgVmCJdf+r7x95TUE7QokpL\n"
"o+uhzpMFAoGBALrdqFzfyrrtjjS0VV3UGRFOUxvXpVAqcyidKJ2wNLfS3dZTCxpq\n"
"LJRTixCpJ7NweJqWss7h2NYU+Kf4AR2QTPFTzpxTrlodouVB7gNzHg8BgWGjtiQW\n"
"HKP0SsJDBThv8choUk9O1lXnZZkWAa0PtLitiZv1wetgLVkhcQsagzZJ\n"
"-----END RSA PRIVATE KEY-----";
#endif
/*
 * PEM-encoded Just-in-Time Registration (JITR) certificate (optional).
 *
 * If used, must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----"
 * "...base64 data..."
 * "-----END CERTIFICATE-----";
 */
static const char * clientcredentialJITR_DEVICE_CERTIFICATE_AUTHORITY_PEM = NULL;
