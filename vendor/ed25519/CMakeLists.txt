set(SRCS
    src/sign.c
    src/precomp_data.h
    src/ge.c
    src/fe.c
    src/sha512.c
    src/seed.c
    src/key_exchange.c
    src/fixedint.h
    src/add_scalar.c
    src/verify.c
    src/ed25519.h
    src/sc.h
    src/ge.h
    src/keypair.c
    src/sha512.h
    src/sc.c
    src/fe.h
)
add_library(ed25519 STATIC ${SRCS})
target_include_directories(ed25519 PUBLIC src)
