#!/bin/bash

hestia_module_nginx_install() {
    source $HESTIA/bin/module/func.inc

    if hestia_module_isinstalled 'nginx' && [ ! "$param_force" ]; then
        echo "Nginx module is already installed. See 'hestia module info nginx'."
        return 0
    fi

    echo "Installing Nginx module..."

    osal_service_stop $OSAL_SERVICE_NGINX > /dev/null 2>&1
    hestia_config_backup 'nginx-install' $OSAL_PATH_NGINX_CONF

    # Setup repos
    if [ "$OS_BASE" = 'debian' ]; then
        cat > /etc/apt/sources.list.d/nginx.list <<EOL
# This file was added by Hestia Control Panel.
deb [arch=amd64] https://nginx.org/packages/mainline/$VERSION/ $OS_CODENAME nginx
EOL
        apt-key adv --fetch-keys 'https://nginx.org/keys/nginx_signing.key' > /dev/null 2>&1
    elif [ "$OS_BASE" = 'ubuntu' ]; then
        cat > /etc/apt/sources.list.d/nginx.list <<EOL
# This file was added by Hestia Control Panel.
deb [arch=amd64] https://nginx.org/packages/mainline/$VERSION/ $OS_CODENAME nginx
EOL
        if [ "$OS_VERSION" = '16.04' ]; then 
            apt-key adv --fetch-keys 'http://nginx.org/keys/nginx_signing.key' > /dev/null 2>&1
        else
            apt-key adv --fetch-keys 'https://nginx.org/keys/nginx_signing.key' > /dev/null 2>&1    
        fi
    elif [ "$OS_BASE" = 'rhel' ]; then
        cat > "/etc/yum.repos.d/nginx.repo" <<EOL
# This file was added by Hestia Control Panel.
[nginx-stable]
name=nginx stable repo
baseurl=http://nginx.org/packages/centos/\$releasever/\$basearch/
gpgcheck=1
enabled=1
gpgkey=https://nginx.org/keys/nginx_signing.key
module_hotfixes=true
EOL
    fi

    osal_package_preinstall
    osal_package_install $OSAL_PKG_NGINX

    hestia_config_backup 'nginx-install' $OSAL_PATH_NGINX_CONF $OSAL_PATH_NGINX_CONF_D
    hestia_safe_rm $OSAL_PATH_NGINX_CONF_D/*.conf
    
    cp -f $HESTIA_INSTALL_DIR/nginx/nginx.conf $OSAL_PATH_NGINX_CONF/
    cp -f $HESTIA_INSTALL_DIR/nginx/status.conf $OSAL_PATH_NGINX_CONF_D/
    #FIXME: move the following to *admin install
    #cp -f $HESTIA_INSTALL_DIR/nginx/phpmyadmin.inc $OSAL_PATH_NGINX_CONF_D/
    #cp -f $HESTIA_INSTALL_DIR/nginx/phppgadmin.inc $OSAL_PATH_NGINX_CONF_D/
    cp -f $HESTIA_INSTALL_DIR/logrotate/nginx $OSAL_PATH_LOGROTATE_CONF_D/
    mkdir -p $OSAL_PATH_NGINX_CONF_D/domains
    mkdir -p $OSAL_PATH_NGINX_CONF/modules-enabled
    mkdir -p /var/log/nginx/domains

    # Update dns servers in nginx.conf
    dns_resolver=$(cat /etc/resolv.conf | grep -i '^nameserver' | cut -d ' ' -f2 | tr '\r\n' ' ' | xargs)
    for ip in $dns_resolver; do
        if [[ $ip =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
            resolver="$ip $resolver"
        fi
    done
    if [ ! -z "$resolver" ]; then
        sed -i "s/1.0.0.1 1.1.1.1/$resolver/g" $OSAL_PATH_NGINX_CONF/nginx.conf
    fi

    osal_kv_write $HESTIA_CONF_MODULES/nginx.conf 'service_name' $OSAL_SERVICE_NGINX
}